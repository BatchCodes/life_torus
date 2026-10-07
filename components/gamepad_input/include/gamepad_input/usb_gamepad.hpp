// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "game/game.hpp"
#include "gamepad_input/report.hpp"

namespace gamepad_input {

struct ButtonEvent {
    game::Button button;
    bool pressed;
};

struct UsbGamepadConfig {
    const ReportLayout* layout = &kGenericSnes;
    bool log_reports = false;  // Log each raw input report as hex. For bring-up.
    int task_priority = 4;
};

// Reads a USB HID gamepad on the ESP32-S3 USB OTG port and sends a ButtonEvent for each press
// and release. When the controller disconnects, it sends a release for each held button.
class UsbGamepad {
public:
    esp_err_t start(const UsbGamepadConfig& config);
    // Waits up to `timeout` for the next event.
    bool next_event(ButtonEvent& event, TickType_t timeout);
    bool connected() const { return connected_; }

    // Called from the USB HID callbacks.
    void on_report(const uint8_t* data, size_t length);
    void on_disconnect();

private:
    void send_changes(ButtonMask buttons);

    UsbGamepadConfig config_;
    QueueHandle_t events_ = nullptr;
    ButtonMask buttons_ = 0;
    volatile bool connected_ = false;
};

}  // namespace gamepad_input
