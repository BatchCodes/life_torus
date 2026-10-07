// SPDX-License-Identifier: GPL-3.0-or-later
#include "app_settings.hpp"

#include <cstring>

#include "esp_random.h"
#include "sdkconfig.h"

settings::Settings settings_from_kconfig() {
    settings::Settings s;
    s.step_ms = CONFIG_LIFE_STEP_MS;
    s.settled_limit = CONFIG_LIFE_SETTLED_LIMIT;
    s.no_input_limit = CONFIG_LIFE_NO_INPUT_LIMIT;
    s.repeat_limit = CONFIG_LIFE_REPEAT_LIMIT;
    s.pause_timeout_ms = CONFIG_LIFE_PAUSE_TIMEOUT_MS;
    s.random_percent = CONFIG_LIFE_RANDOM_PERCENT;
#if CONFIG_LIFE_EDGE_CYLINDER
    s.cylinder = true;
#endif
    s.intensity = CONFIG_LIFE_INTENSITY;
#if CONFIG_LIFE_BRIGHTNESS_LEVELS
    s.brightness_levels = true;
#else
    s.brightness_levels = false;
#endif
#ifdef CONFIG_LIFE_PANEL_REVERSE_RING
    s.panel.reverse_ring = true;
#endif
#ifdef CONFIG_LIFE_PANEL_FLIP_BOARDS
    s.panel.flip_boards = true;
#endif
#ifdef CONFIG_LIFE_PANEL_ZIGZAG
    s.panel.zigzag = true;
#endif
#ifdef CONFIG_LIFE_PANEL_BLOCK_TRANSPOSE
    s.panel.block_transpose = true;
#endif
#ifdef CONFIG_LIFE_PANEL_BLOCK_FLIP_X
    s.panel.block_flip_x = true;
#endif
#ifdef CONFIG_LIFE_PANEL_BLOCK_FLIP_Y
    s.panel.block_flip_y = true;
#endif
    std::strncpy(s.password, CONFIG_LIFE_WEB_PASSWORD, settings::kMaxPassword);
    s.password[settings::kMaxPassword] = '\0';
    settings::clamp(s);
    return s;
}

game::GameConfig game_config(const settings::Settings& s) {
    game::GameConfig config;
    config.transition_ms = CONFIG_LIFE_TRANSITION_MS;
    config.cursor_blink_ms = CONFIG_LIFE_CURSOR_BLINK_MS;
    config.repeat_delay_ms = CONFIG_LIFE_DPAD_REPEAT_DELAY_MS;
    config.repeat_interval_ms = CONFIG_LIFE_DPAD_REPEAT_INTERVAL_MS;
    config.ko_effect_ms = CONFIG_LIFE_KO_SCROLL_MS;
    config.ko_gap_ms = CONFIG_LIFE_KO_GAP_MS;
    config.seed = esp_random();
    settings::apply(s, config);
    return config;
}
