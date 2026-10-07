// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdint>

#include "board.hpp"
#include "esp_log.h"
#include "esp_random.h"
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

game::GameConfig config_from_kconfig(bool levels) {
    game::GameConfig config;
    config.step_ms = CONFIG_LIFE_STEP_MS;
    config.brightness_levels = levels;
    config.empty_limit = CONFIG_LIFE_EMPTY_LIMIT;
    config.no_input_limit = CONFIG_LIFE_NO_INPUT_LIMIT;
    config.repeat_limit = CONFIG_LIFE_REPEAT_LIMIT;
    config.pause_timeout_ms = CONFIG_LIFE_PAUSE_TIMEOUT_MS;
    config.transition_ms = CONFIG_LIFE_TRANSITION_MS;
    config.cursor_blink_ms = CONFIG_LIFE_CURSOR_BLINK_MS;
    config.repeat_delay_ms = CONFIG_LIFE_DPAD_REPEAT_DELAY_MS;
    config.repeat_interval_ms = CONFIG_LIFE_DPAD_REPEAT_INTERVAL_MS;
    config.ko_effect_ms = CONFIG_LIFE_KO_SCROLL_MS;
    config.ko_gap_ms = CONFIG_LIFE_KO_GAP_MS;
#if CONFIG_LIFE_EDGE_CYLINDER
    config.default_edge_mode = life::EdgeMode::kCylinder;
#else
    config.default_edge_mode = life::EdgeMode::kTorus;
#endif
    config.random_percent = CONFIG_LIFE_RANDOM_PERCENT;
    config.seed = esp_random();
    return config;
}

uint32_t now_ms() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000);
}

}  // namespace

void run_game(Board& board) {
    gamepad_input::UsbGamepad gamepad;
    gamepad_input::UsbGamepadConfig pad_config;
#if CONFIG_LIFE_LOG_CONTROLLER_REPORTS
    pad_config.log_reports = true;
#endif
    if (gamepad.start(pad_config) != ESP_OK) {
        ESP_LOGW(kTag, "USB host start failed. The display runs with no controller.");
    }

    game::Game game(config_from_kconfig(board.levels()));
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
