// SPDX-License-Identifier: GPL-3.0-or-later
#include "phone_link/phone_link.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_wifi.h"

namespace phone_link {

namespace {

constexpr const char* kTag = "phone_link";
constexpr const char* kCookie = "session";
constexpr size_t kMaxMessage = kMaxCommandText + 16;

PhoneLink* g_link = nullptr;

esp_err_t file_handler(httpd_req_t* req) {
    return g_link->handle_file(req);
}
esp_err_t login_handler(httpd_req_t* req) {
    return g_link->handle_login(req);
}
esp_err_t ws_handler(httpd_req_t* req) {
    return g_link->handle_ws(req);
}
void close_handler(httpd_handle_t /*hd*/, int fd) {
    g_link->on_close(fd);
    close(fd);
}

struct SendJob {
    httpd_handle_t server;
    httpd_ws_type_t type;
    int fds[PhoneLink::kMaxClients];
    size_t length;
    uint8_t data[1];  // The message follows the struct.
};

void send_job(void* arg) {
    auto* job = static_cast<SendJob*>(arg);
    httpd_ws_frame_t frame = {};
    frame.type = job->type;
    frame.payload = job->data;
    frame.len = job->length;
    for (int fd : job->fds) {
        if (fd >= 0 && httpd_ws_get_fd_info(job->server, fd) == HTTPD_WS_CLIENT_WEBSOCKET) {
            httpd_ws_send_frame_async(job->server, fd, &frame);
        }
    }
    std::free(job);
}

esp_err_t start_wifi(const char* ssid, uint32_t& ip_address) {
    ESP_RETURN_ON_ERROR(esp_netif_init(), kTag, "netif init failed");
    esp_err_t err = esp_event_loop_create_default();
    ESP_RETURN_ON_FALSE(err == ESP_OK || err == ESP_ERR_INVALID_STATE, err, kTag,
                        "event loop failed");
    esp_netif_t* netif = esp_netif_create_default_wifi_ap();
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init), kTag, "Wi-Fi init failed");

    wifi_config_t config = {};
    const size_t length = std::strlen(ssid);
    std::memcpy(config.ap.ssid, ssid, length < sizeof(config.ap.ssid) ? length : 31);
    config.ap.ssid_len = static_cast<uint8_t>(length < 32 ? length : 32);
    config.ap.channel = 6;
    config.ap.authmode = WIFI_AUTH_OPEN;
    config.ap.max_connection = PhoneLink::kMaxClients;
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), kTag, "Wi-Fi mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &config), kTag, "Wi-Fi config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), kTag, "Wi-Fi start failed");

    esp_netif_ip_info_t ip = {};
    ESP_RETURN_ON_ERROR(esp_netif_get_ip_info(netif, &ip), kTag, "IP info failed");
    ip_address = ip.ip.addr;
    ESP_LOGI(kTag, "Wi-Fi network \"%s\" at " IPSTR, ssid, IP2STR(&ip.ip));
    return ESP_OK;
}

}  // namespace

esp_err_t PhoneLink::start(const PhoneLinkConfig& config, const char* password) {
    config_ = config;
    g_link = this;
    lock_ = xSemaphoreCreateMutex();
    commands_ = xQueueCreate(8, sizeof(Command));
    ESP_RETURN_ON_FALSE(lock_ != nullptr && commands_ != nullptr, ESP_ERR_NO_MEM, kTag,
                        "queue create failed");
    set_password(password);

    uint32_t ip_address = 0;
    ESP_RETURN_ON_ERROR(start_wifi(config_.ssid, ip_address), kTag, "Wi-Fi failed");
    ESP_RETURN_ON_ERROR(start_captive_dns(ip_address), kTag, "DNS failed");

    httpd_config_t http = HTTPD_DEFAULT_CONFIG();
    http.max_uri_handlers = 8;
    http.max_open_sockets = 7;
    http.lru_purge_enable = true;
    http.stack_size = 6144;
    http.uri_match_fn = httpd_uri_match_wildcard;
    http.close_fn = close_handler;
    ESP_RETURN_ON_ERROR(httpd_start(&server_, &http), kTag, "HTTP server start failed");

    httpd_uri_t ws = {};
    ws.uri = "/ws";
    ws.method = HTTP_GET;
    ws.handler = ws_handler;
    ws.is_websocket = true;
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server_, &ws), kTag, "WebSocket failed");
    httpd_uri_t login = {};
    login.uri = "/login";
    login.method = HTTP_POST;
    login.handler = login_handler;
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server_, &login), kTag, "login failed");
    httpd_uri_t files = {};
    files.uri = "/*";
    files.method = HTTP_GET;
    files.handler = file_handler;
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server_, &files), kTag, "files failed");
    return ESP_OK;
}

void PhoneLink::set_password(const char* password) {
    xSemaphoreTake(lock_, portMAX_DELAY);
    std::strncpy(password_, password, sizeof(password_) - 1);
    password_[sizeof(password_) - 1] = '\0';
    sessions_.clear();  // A new password logs out every phone.
    xSemaphoreGive(lock_);
}

bool PhoneLink::next_command(Command& command, TickType_t timeout) {
    return xQueueReceive(commands_, &command, timeout) == pdTRUE;
}

esp_err_t PhoneLink::handle_file(httpd_req_t* req) {
    const StaticFile* file = nullptr;
    if (std::strcmp(req->uri, "/") == 0 && config_.file_count > 0) {
        file = &config_.files[0];
    }
    for (size_t i = 0; file == nullptr && i < config_.file_count; ++i) {
        if (std::strcmp(req->uri, config_.files[i].uri) == 0) {
            file = &config_.files[i];
        }
    }
    if (file == nullptr) {
        // Captive portal: a phone checks a known address to find out if it is online. Send it
        // to the web app instead.
        httpd_resp_set_status(req, "302 Found");
        httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
        return httpd_resp_send(req, nullptr, 0);
    }
    httpd_resp_set_type(req, file->content_type);
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    return httpd_resp_send(req, reinterpret_cast<const char*>(file->start),
                           file->end - file->start);
}

esp_err_t PhoneLink::handle_login(httpd_req_t* req) {
    char body[160] = {};
    if (req->content_len >= sizeof(body)) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "too long");
    }
    int received = 0;
    while (received < static_cast<int>(req->content_len)) {
        const int n = httpd_req_recv(req, body + received, req->content_len - received);
        if (n <= 0) {
            return ESP_FAIL;
        }
        received += n;
    }
    // The body is "password=VALUE", percent-encoded.
    char given[80] = {};
    const char* prefix = "password=";
    const size_t prefix_length = std::strlen(prefix);
    if (std::strncmp(body, prefix, prefix_length) != 0 ||
        !url_decode(body + prefix_length, std::strlen(body) - prefix_length, given,
                    sizeof(given))) {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "bad request");
    }

    xSemaphoreTake(lock_, portMAX_DELAY);
    const bool ok = password_matches(password_, given, std::strlen(given));
    char cookie[80] = {};
    if (ok) {
        uint8_t random[16];
        esp_fill_random(random, sizeof(random));
        std::snprintf(cookie, sizeof(cookie), "%s=%s; Path=/; HttpOnly; SameSite=Strict", kCookie,
                      sessions_.create(random));
    }
    xSemaphoreGive(lock_);

    if (!ok) {
        httpd_resp_set_status(req, "403 Forbidden");
        return httpd_resp_sendstr(req, "wrong password");
    }
    httpd_resp_set_hdr(req, "Set-Cookie", cookie);
    return httpd_resp_sendstr(req, "ok");
}

bool PhoneLink::session_ok(httpd_req_t* req) {
    char header[160] = {};
    if (httpd_req_get_hdr_value_str(req, "Cookie", header, sizeof(header)) != ESP_OK) {
        return false;
    }
    char token[Sessions::kTokenLength + 2] = {};
    if (!find_cookie(header, kCookie, token, sizeof(token))) {
        return false;
    }
    xSemaphoreTake(lock_, portMAX_DELAY);
    const bool ok = sessions_.valid(token, std::strlen(token));
    xSemaphoreGive(lock_);
    return ok;
}

void PhoneLink::add_client(int fd) {
    xSemaphoreTake(lock_, portMAX_DELAY);
    int count = 0;
    bool added = false;
    for (int& client : clients_) {
        if (client == fd) {
            added = true;
        }
    }
    for (int& client : clients_) {
        if (!added && client < 0) {
            client = fd;
            added = true;
        }
        count += client >= 0 ? 1 : 0;
    }
    client_count_ = count;
    xSemaphoreGive(lock_);
}

void PhoneLink::on_close(int fd) {
    xSemaphoreTake(lock_, portMAX_DELAY);
    int count = 0;
    for (int& client : clients_) {
        if (client == fd) {
            client = -1;
        }
        count += client >= 0 ? 1 : 0;
    }
    client_count_ = count;
    xSemaphoreGive(lock_);
}

esp_err_t PhoneLink::handle_ws(httpd_req_t* req) {
    if (req->method == HTTP_GET) {
        // The WebSocket handshake. Only a logged-in phone gets a connection.
        if (!session_ok(req)) {
            ESP_LOGW(kTag, "WebSocket refused: not logged in");
            return ESP_FAIL;
        }
        add_client(httpd_req_to_sockfd(req));
        ESP_LOGI(kTag, "phone connected (%d)", client_count_);
        return ESP_OK;
    }

    httpd_ws_frame_t frame = {};
    ESP_RETURN_ON_ERROR(httpd_ws_recv_frame(req, &frame, 0), kTag, "frame length failed");
    if (frame.type != HTTPD_WS_TYPE_TEXT || frame.len == 0 || frame.len > kMaxMessage) {
        return ESP_OK;
    }
    uint8_t data[kMaxMessage + 1];
    frame.payload = data;
    ESP_RETURN_ON_ERROR(httpd_ws_recv_frame(req, &frame, frame.len), kTag, "frame read failed");
    Command command;
    if (parse_command(reinterpret_cast<const char*>(data), frame.len, command)) {
        xQueueSend(commands_, &command, 0);
    } else {
        ESP_LOGW(kTag, "unknown message from the phone");
    }
    return ESP_OK;
}

void PhoneLink::send_all(httpd_ws_type_t type, const uint8_t* data, size_t length) {
    if (client_count_ == 0 || server_ == nullptr) {
        return;
    }
    auto* job = static_cast<SendJob*>(std::malloc(sizeof(SendJob) + length));
    if (job == nullptr) {
        return;
    }
    job->server = server_;
    job->type = type;
    job->length = length;
    std::memcpy(job->data, data, length);
    xSemaphoreTake(lock_, portMAX_DELAY);
    for (int i = 0; i < kMaxClients; ++i) {
        job->fds[i] = clients_[i];
    }
    xSemaphoreGive(lock_);
    if (httpd_queue_work(server_, send_job, job) != ESP_OK) {
        std::free(job);
    }
}

void PhoneLink::send_frame(const frame::Image& image) {
    uint8_t data[kMaxFrameSize];
    pack_frame(image, data);
    send_all(HTTPD_WS_TYPE_BINARY, data, frame_size());
}

void PhoneLink::send_rgb_frame(const colour::RgbImage& image) {
    if (client_count_ == 0) {
        return;
    }
    auto* data = static_cast<uint8_t*>(std::malloc(kMaxRgbFrameSize));
    if (data == nullptr) {
        return;
    }
    pack_rgb_frame(image, data);
    send_all(HTTPD_WS_TYPE_BINARY, data, rgb_frame_size());
    std::free(data);
}

void PhoneLink::send_text(const char* text) {
    send_all(HTTPD_WS_TYPE_TEXT, reinterpret_cast<const uint8_t*>(text), std::strlen(text));
}

}  // namespace phone_link
