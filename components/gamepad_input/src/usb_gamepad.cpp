// SPDX-License-Identifier: GPL-3.0-or-later
#include "gamepad_input/usb_gamepad.hpp"

#include <cstdio>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "usb/hid_host.h"
#include "usb/usb_host.h"

namespace gamepad_input {

namespace {

constexpr const char* kTag = "gamepad";
constexpr size_t kMaxReport = 64;

UsbGamepad* g_gamepad = nullptr;

void usb_lib_task(void* /*arg*/) {
    while (true) {
        uint32_t flags = 0;
        usb_host_lib_handle_events(portMAX_DELAY, &flags);
        if (flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
    }
}

void interface_callback(hid_host_device_handle_t handle, const hid_host_interface_event_t event,
                        void* /*arg*/) {
    switch (event) {
        case HID_HOST_INTERFACE_EVENT_INPUT_REPORT: {
            uint8_t data[kMaxReport];
            size_t length = 0;
            if (hid_host_device_get_raw_input_report_data(handle, data, sizeof(data), &length) ==
                ESP_OK) {
                g_gamepad->on_report(data, length);
            }
            break;
        }
        case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
            ESP_LOGI(kTag, "controller disconnected");
            g_gamepad->on_disconnect();
            hid_host_device_close(handle);
            break;
        case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
            ESP_LOGW(kTag, "transfer error");
            break;
        default:
            break;
    }
}

void driver_callback(hid_host_device_handle_t handle, const hid_host_driver_event_t event,
                     void* /*arg*/) {
    if (event != HID_HOST_DRIVER_EVENT_CONNECTED) {
        return;
    }
    hid_host_dev_params_t params = {};
    hid_host_device_get_params(handle, &params);
    ESP_LOGI(kTag, "HID interface connected: address %u, interface %u, protocol %u", params.addr,
             params.iface_num, params.proto);
    hid_host_device_config_t device_config = {};
    device_config.callback = interface_callback;
    if (hid_host_device_open(handle, &device_config) != ESP_OK) {
        ESP_LOGW(kTag, "HID interface open failed");
        return;
    }
    if (hid_host_device_start(handle) != ESP_OK) {
        ESP_LOGW(kTag, "HID interface start failed");
    }
}

}  // namespace

esp_err_t UsbGamepad::start(const UsbGamepadConfig& config) {
    config_ = config;
    g_gamepad = this;
    events_ = xQueueCreate(32, sizeof(ButtonEvent));
    ESP_RETURN_ON_FALSE(events_ != nullptr, ESP_ERR_NO_MEM, kTag, "queue create failed");

    usb_host_config_t host_config = {};
    host_config.intr_flags = 0;
    ESP_RETURN_ON_ERROR(usb_host_install(&host_config), kTag, "USB host install failed");
    ESP_RETURN_ON_FALSE(xTaskCreate(usb_lib_task, "usb_lib", 4096, nullptr, config_.task_priority,
                                    nullptr) == pdPASS,
                        ESP_ERR_NO_MEM, kTag, "USB task create failed");

    hid_host_driver_config_t hid_config = {};
    hid_config.create_background_task = true;
    hid_config.task_priority = config_.task_priority;
    hid_config.stack_size = 4096;
    hid_config.core_id = tskNO_AFFINITY;
    hid_config.callback = driver_callback;
    ESP_RETURN_ON_ERROR(hid_host_install(&hid_config), kTag, "HID host install failed");
    ESP_LOGI(kTag, "waiting for a USB controller (%s layout)", config_.layout->name);
    return ESP_OK;
}

bool UsbGamepad::next_event(ButtonEvent& event, TickType_t timeout) {
    return xQueueReceive(events_, &event, timeout) == pdTRUE;
}

void UsbGamepad::on_report(const uint8_t* data, size_t length) {
    if (!connected_) {
        connected_ = true;
        ESP_LOGI(kTag, "controller connected");
    }
    if (config_.log_reports) {
        char text[kMaxReport * 3 + 1] = {};
        for (size_t i = 0; i < length && i < kMaxReport; ++i) {
            std::snprintf(text + i * 3, 4, "%02X ", data[i]);
        }
        ESP_LOGI(kTag, "report (%u bytes): %s", static_cast<unsigned>(length), text);
    }
    ButtonMask buttons = 0;
    if (parse_report(*config_.layout, data, length, buttons)) {
        send_changes(buttons);
    }
}

void UsbGamepad::on_disconnect() {
    connected_ = false;
    send_changes(0);
}

void UsbGamepad::send_changes(ButtonMask buttons) {
    for_each_change(buttons_, buttons, [this](game::Button button, bool pressed) {
        const ButtonEvent event{button, pressed};
        xQueueSend(events_, &event, 0);
    });
    buttons_ = buttons;
}

}  // namespace gamepad_input
