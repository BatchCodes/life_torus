// SPDX-License-Identifier: GPL-3.0-or-later
#include "mode_button/mode_button.hpp"

#include "driver/gpio.h"
#include "esp_check.h"

namespace mode_button {

namespace {

constexpr const char* kTag = "mode_button";

}  // namespace

esp_err_t ModeButton::start(int gpio) {
    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << gpio;
    config.mode = GPIO_MODE_INPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    ESP_RETURN_ON_ERROR(gpio_config(&config), kTag, "GPIO config failed");
    gpio_ = gpio;
    return ESP_OK;
}

bool ModeButton::poll(uint32_t now_ms) {
    if (gpio_ < 0) {
        return false;
    }
    // Pressed connects the GPIO to GND.
    const bool pressed = gpio_get_level(static_cast<gpio_num_t>(gpio_)) == 0;
    return debouncer_.update(pressed, now_ms);
}

}  // namespace mode_button
