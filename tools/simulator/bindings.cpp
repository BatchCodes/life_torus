// SPDX-License-Identifier: GPL-3.0-or-later
//
// C functions for the JavaScript page. The page owns the clock and calls these functions with
// performance.now() in milliseconds.
#include <emscripten/emscripten.h>

#include <cmath>
#include <cstdint>

#include "beat/beat_detector.hpp"
#include "display_modes/mode_manager.hpp"
#include "frame/image.hpp"
#include "game/game.hpp"
#include "patterns/library.hpp"
#include "patterns/presets.hpp"
#include "spectrum/analyser.hpp"

namespace {

game::Game g_game{game::GameConfig{}};
display_modes::ModeManager g_modes{g_game, 12345};
frame::Image g_image;
uint8_t g_levels[life::kWidth * life::kHeight];

// Beat sync with the computer or phone microphone (Web Audio).
constexpr int kAudioBuffer = 8192;
float g_audio[kAudioBuffer];
beat::BeatDetector g_beat{48000};
spectrum::Analyser g_spectrum{48000};
uint32_t g_spectrum_seen = 0;
bool g_listening = false;
bool g_beat_sync = true;
uint32_t g_last_audio_ms = 0;
float g_audio_level = 0;  // RMS of the last audio block.

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
    if (g_listening && g_beat_sync) {
        // Map the page clock to the audio time of the detector.
        const uint32_t audio_ms = g_beat.time_ms() + (now_ms - g_last_audio_ms);
        if (g_beat.take_beat(audio_ms)) {
            g_modes.beat(now_ms);
        }
    }
    if (g_listening && g_spectrum.active() && g_spectrum.updates() != g_spectrum_seen) {
        g_spectrum_seen = g_spectrum.updates();
        g_modes.set_spectrum(g_spectrum.bands().data(), now_ms);
    }
    g_modes.tick(now_ms);
}

// Starts beat detection at the sample rate of the page's audio input.
EMSCRIPTEN_KEEPALIVE void sim_listen(int sample_rate) {
    g_beat = beat::BeatDetector(sample_rate);
    g_spectrum = spectrum::Analyser(sample_rate);
    g_listening = true;
}

EMSCRIPTEN_KEEPALIVE float* sim_audio_buffer() {
    return g_audio;
}

EMSCRIPTEN_KEEPALIVE void sim_audio_process(int count, uint32_t now_ms) {
    if (count > 0 && count <= kAudioBuffer) {
        g_beat.process(g_audio, static_cast<size_t>(count));
        g_spectrum.process(g_audio, static_cast<size_t>(count));
        float sum = 0;
        for (int i = 0; i < count; ++i) {
            sum += g_audio[i] * g_audio[i];
        }
        g_audio_level = std::sqrt(sum / static_cast<float>(count));
        g_last_audio_ms = now_ms;
    }
}

EMSCRIPTEN_KEEPALIVE void sim_set_beat(int beat_sync, int sensitivity) {
    g_beat_sync = beat_sync != 0;
    g_beat.set_sensitivity(sensitivity);
}

EMSCRIPTEN_KEEPALIVE int sim_bpm() {
    return g_listening && g_beat.stable() ? static_cast<int>(g_beat.bpm() + 0.5f) : 0;
}

// The input level, 0 to 100, on a dB scale from -60 dBFS to 0 dBFS.
EMSCRIPTEN_KEEPALIVE int sim_audio_level() {
    const float db = 20.0f * std::log10(g_audio_level + 1e-9f);
    const float level = (db + 60.0f) / 60.0f * 100.0f;
    return level < 0 ? 0 : (level > 100 ? 100 : static_cast<int>(level));
}

EMSCRIPTEN_KEEPALIVE int sim_listening() {
    return g_listening ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE void sim_set_mode(int mode, uint32_t now_ms) {
    if (mode >= 0 && mode < display_modes::kModeCount) {
        g_modes.set_mode(static_cast<display_modes::ModeId>(mode), now_ms);
    }
}

EMSCRIPTEN_KEEPALIVE void sim_next_mode(uint32_t now_ms) {
    g_modes.next_mode(now_ms);
}

EMSCRIPTEN_KEEPALIVE void sim_set_text(const char* text) {
    g_modes.set_text(text);
}

EMSCRIPTEN_KEEPALIVE void sim_set_speed(int speed) {
    g_modes.set_speed(speed);
}

EMSCRIPTEN_KEEPALIVE void sim_tap_cell(int x, int y, uint32_t now_ms) {
    if (g_modes.mode() == display_modes::ModeId::kGameOfLife) {
        g_game.tap_cell(x, y, now_ms);
    }
}

EMSCRIPTEN_KEEPALIVE void sim_select_preset(int index, uint32_t now_ms) {
    if (g_modes.mode() == display_modes::ModeId::kGameOfLife) {
        g_game.select_preset(index, now_ms);
    }
}

EMSCRIPTEN_KEEPALIVE int sim_preset_index() {
    return g_game.preset_index();
}

EMSCRIPTEN_KEEPALIVE int sim_preset_count() {
    return static_cast<int>(patterns::presets().size());
}

EMSCRIPTEN_KEEPALIVE const char* sim_preset_name_at(int index) {
    const auto all = patterns::presets();
    return index >= 0 && index < static_cast<int>(all.size()) ? all[index].name : "";
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
