// SPDX-License-Identifier: GPL-3.0-or-later
//
// C functions for the JavaScript page. The page owns the clock and calls these functions with
// performance.now() in milliseconds.
#include <emscripten/emscripten.h>

#include <cstdint>

#include "display_modes/mode_manager.hpp"
#include "frame/image.hpp"
#include "game/game.hpp"
#include "patterns/library.hpp"

namespace {

game::Game g_game{game::GameConfig{}};
display_modes::ModeManager g_modes{g_game, 12345};
frame::Image g_image;
uint8_t g_levels[life::kWidth * life::kHeight];

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE void sim_configure(uint32_t step_ms, int brightness_levels,
                                        uint32_t settled_limit, uint32_t no_input_limit,
                                        uint32_t repeat_limit, uint32_t pause_timeout_ms,
                                        uint32_t ko_effect_ms, int cylinder, int random_percent,
                                        uint32_t seed) {
    game::GameConfig config;
    config.step_ms = step_ms;
    config.brightness_levels = brightness_levels != 0;
    config.settled_limit = settled_limit;
    config.no_input_limit = no_input_limit;
    config.repeat_limit = repeat_limit;
    config.pause_timeout_ms = pause_timeout_ms;
    config.ko_effect_ms = ko_effect_ms;
    config.default_edge_mode = cylinder != 0 ? life::EdgeMode::kCylinder : life::EdgeMode::kTorus;
    config.random_percent = random_percent;
    config.seed = seed;
    g_game.set_config(config);
}

EMSCRIPTEN_KEEPALIVE void sim_start(uint32_t now_ms) {
    g_game.start(now_ms);
}

EMSCRIPTEN_KEEPALIVE void sim_press(int button, uint32_t now_ms) {
    g_modes.press(static_cast<game::Button>(button), now_ms);
}

EMSCRIPTEN_KEEPALIVE void sim_release(int button, uint32_t now_ms) {
    g_modes.release(static_cast<game::Button>(button), now_ms);
}

EMSCRIPTEN_KEEPALIVE void sim_tick(uint32_t now_ms) {
    g_modes.tick(now_ms);
}

EMSCRIPTEN_KEEPALIVE void sim_set_mode(int mode, uint32_t now_ms) {
    if (mode >= 0 && mode < display_modes::kModeCount) {
        g_modes.set_mode(static_cast<display_modes::ModeId>(mode), now_ms);
    }
}

EMSCRIPTEN_KEEPALIVE void sim_set_text(const char* text) {
    g_modes.set_text(text);
}

EMSCRIPTEN_KEEPALIVE void sim_set_speed(int speed) {
    g_modes.set_speed(speed);
}

EMSCRIPTEN_KEEPALIVE int sim_display_mode() {
    return static_cast<int>(g_modes.mode());
}

// Renders the display and returns 64 × 32 brightness levels (0 to 3), row by row.
EMSCRIPTEN_KEEPALIVE const uint8_t* sim_render(uint32_t now_ms) {
    g_modes.render(g_image, now_ms);
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::kWidth; ++x) {
            g_levels[y * life::kWidth + x] = static_cast<uint8_t>(g_image.get(x, y));
        }
    }
    return g_levels;
}

EMSCRIPTEN_KEEPALIVE int sim_mode() {
    return static_cast<int>(g_game.mode());
}
EMSCRIPTEN_KEEPALIVE uint32_t sim_generation() {
    return g_game.simulation().generation();
}
EMSCRIPTEN_KEEPALIVE int sim_population() {
    return g_game.simulation().current().population();
}
EMSCRIPTEN_KEEPALIVE int sim_cylinder() {
    return g_game.simulation().edge_mode() == life::EdgeMode::kCylinder ? 1 : 0;
}
EMSCRIPTEN_KEEPALIVE const char* sim_shape_name() {
    return g_game.shape_name();
}
EMSCRIPTEN_KEEPALIVE const char* sim_preset_name() {
    return g_game.preset_name();
}
EMSCRIPTEN_KEEPALIVE int sim_cursor_x() {
    return g_game.cursor_x();
}

}  // extern "C"
