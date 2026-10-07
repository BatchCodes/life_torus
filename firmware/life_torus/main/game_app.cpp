// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdint>

#include "app_settings.hpp"
#include "board.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "game/game.hpp"
#include "gamepad_input/usb_gamepad.hpp"
#include "modes.hpp"
#include "sdkconfig.h"

namespace {

constexpr const char* kTag = "game";
constexpr uint32_t kTickMs = 10;
constexpr uint32_t kRenderMs = 20;

uint32_t now_ms() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000);
}

}  // namespace

void run_game(Board& board, settings::Settings& s) {
    gamepad_input::UsbGamepad gamepad;
    gamepad_input::UsbGamepadConfig pad_config;
#if CONFIG_LIFE_LOG_CONTROLLER_REPORTS
    pad_config.log_reports = true;
#endif
    if (gamepad.start(pad_config) != ESP_OK) {
        ESP_LOGW(kTag, "USB host start failed. The display runs with no controller.");
    }

    game::Game game(game_config(s));
    game.start(now_ms());
    ESP_LOGI(kTag, "started with preset \"%s\"", game.preset_name());

    frame::Image image;
    uint32_t last_render = 0;
    int last_preset = game.preset_index();
    TickType_t wake = xTaskGetTickCount();
    while (true) {
        gamepad_input::ButtonEvent event;
        while (gamepad.next_event(event, 0)) {
            if (event.pressed) {
                game.press(event.button, now_ms());
            } else {
                game.release(event.button, now_ms());
            }
        }
        const uint32_t now = now_ms();
        game.tick(now);
        if (game.preset_index() != last_preset) {
            last_preset = game.preset_index();
            ESP_LOGI(kTag, "preset \"%s\"", game.preset_name());
        }
        if (now - last_render >= kRenderMs) {
            last_render = now;
            game.render(image, now);
            board.show(image);
        }
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(kTickMs));
    }
}
