// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"
#include "esp_http_server.h"
#include "frame/image.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "phone_link/protocol.hpp"

namespace phone_link {

// A file of the web app, embedded in the firmware.
struct StaticFile {
    const char* uri;  // For example "/app.js". The first file is also served at "/".
    const char* content_type;
    const uint8_t* start;
    const uint8_t* end;
};

struct PhoneLinkConfig {
    const char* ssid = "LIFE-TORUS";
    const StaticFile* files = nullptr;
    size_t file_count = 0;
};

// The phone connection: an open Wi-Fi access point, a captive portal that opens the web app, a
// password login and a WebSocket for commands, display frames and status messages.
class PhoneLink {
public:
    static constexpr int kMaxClients = 4;

    esp_err_t start(const PhoneLinkConfig& config, const char* password);
    void set_password(const char* password);

    // Waits up to `timeout` for the next command from a phone.
    bool next_command(Command& command, TickType_t timeout);
    // Sends a display frame or a text message to every logged-in phone.
    void send_frame(const frame::Image& image);
    void send_rgb_frame(const colour::RgbImage& image);
    void send_text(const char* text);
    int client_count() const { return client_count_; }

    // Called from the HTTP server task.
    esp_err_t handle_file(httpd_req_t* req);
    esp_err_t handle_login(httpd_req_t* req);
    esp_err_t handle_ws(httpd_req_t* req);
    void on_close(int fd);

private:
    bool session_ok(httpd_req_t* req);
    void add_client(int fd);
    void send_all(httpd_ws_type_t type, const uint8_t* data, size_t length);

    PhoneLinkConfig config_;
    httpd_handle_t server_ = nullptr;
    QueueHandle_t commands_ = nullptr;
    SemaphoreHandle_t lock_ = nullptr;
    char password_[64] = {};
    Sessions sessions_;
    int clients_[kMaxClients] = {-1, -1, -1, -1};
    volatile int client_count_ = 0;
};

// Answers every DNS query with the address of the access point, so a phone opens the captive
// portal. Runs in its own task.
esp_err_t start_captive_dns(uint32_t ip_address);

}  // namespace phone_link
