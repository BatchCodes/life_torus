// SPDX-License-Identifier: GPL-3.0-or-later
//
// A minimal DNS server for the captive portal (RFC 1035): every A query gets the address of the
// access point.
#include <cstring>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "phone_link/phone_link.hpp"

namespace phone_link {

namespace {

constexpr const char* kTag = "dns";
constexpr int kPort = 53;
constexpr size_t kMaxPacket = 512;

uint32_t g_address = 0;  // Network byte order.

void dns_task(void* /*arg*/) {
    const int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(kTag, "socket failed");
        vTaskDelete(nullptr);
        return;
    }
    sockaddr_in local = {};
    local.sin_family = AF_INET;
    local.sin_port = htons(kPort);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, reinterpret_cast<sockaddr*>(&local), sizeof(local)) < 0) {
        ESP_LOGE(kTag, "bind failed");
        close(sock);
        vTaskDelete(nullptr);
        return;
    }

    uint8_t packet[kMaxPacket + 16];
    while (true) {
        sockaddr_in from = {};
        socklen_t from_length = sizeof(from);
        const int length =
            recvfrom(sock, packet, kMaxPacket, 0, reinterpret_cast<sockaddr*>(&from), &from_length);
        // A query has a 12-byte header and one question.
        if (length < 12 || (packet[2] & 0x80) != 0 || packet[4] != 0 || packet[5] != 1) {
            continue;
        }
        // Find the end of the question: the name, then type and class (4 bytes).
        int p = 12;
        while (p < length && packet[p] != 0) {
            p += packet[p] + 1;
        }
        p += 5;
        if (p > length) {
            continue;
        }
        packet[2] = 0x84;  // Response, authoritative.
        packet[3] = 0x00;  // No error.
        packet[8] = packet[9] = packet[10] = packet[11] = 0;
        const bool type_a = packet[p - 4] == 0 && packet[p - 3] == 1;
        packet[6] = 0;
        packet[7] = type_a ? 1 : 0;  // Answer A queries only. Other types get no answer.
        if (!type_a) {
            sendto(sock, packet, p, 0, reinterpret_cast<sockaddr*>(&from), from_length);
            continue;
        }
        const uint8_t answer[] = {
            0xC0, 0x0C,              // Name: pointer to the question.
            0x00, 0x01, 0x00, 0x01,  // Type A, class IN.
            0x00, 0x00, 0x00, 0x3C,  // TTL 60 s.
            0x00, 0x04,              // Address length.
        };
        std::memcpy(packet + p, answer, sizeof(answer));
        std::memcpy(packet + p + sizeof(answer), &g_address, 4);
        sendto(sock, packet, p + sizeof(answer) + 4, 0, reinterpret_cast<sockaddr*>(&from),
               from_length);
    }
}

}  // namespace

esp_err_t start_captive_dns(uint32_t ip_address) {
    g_address = ip_address;
    ESP_RETURN_ON_FALSE(xTaskCreate(dns_task, "dns", 3072, nullptr, 3, nullptr) == pdPASS,
                        ESP_ERR_NO_MEM, kTag, "task create failed");
    return ESP_OK;
}

}  // namespace phone_link
