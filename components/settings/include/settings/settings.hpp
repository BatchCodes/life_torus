// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>

#include "game/config.hpp"
#include "panel_map/panel_map.hpp"

namespace settings {

constexpr size_t kMaxPassword = 32;
constexpr size_t kMaxText = 600;  // Longest settings text.

// The options that a person can change at run time, from the phone app. The firmware fills the
// defaults from Kconfig, then applies the saved settings from NVS.
struct Settings {
    // Game.
    uint32_t step_ms = 100;
    uint32_t settled_limit = 30;
    uint32_t no_input_limit = 10000;
    uint32_t repeat_limit = 300;
    uint32_t pause_timeout_ms = 30000;
    uint32_t random_percent = 30;
    bool cylinder = false;  // Edges of the empty and random presets.
    // Display.
    uint32_t boards = 8;  // 1 to 16 boards of 32 × 8. A change applies after a restart.
    uint32_t intensity = 4;
    bool brightness_levels = true;
    panel_map::PanelConfig panel;
    // Beat sync of the display modes, with a microphone.
    bool beat_sync = true;
    uint32_t beat_sensitivity = 5;  // 1 to 10.
    // Phone app login.
    char password[kMaxPassword + 1] = "life";
};

// Limits each value to its allowed range.
void clamp(Settings& s);

// Writes the settings as "key=value&key=value", with percent encoding, for NVS and the phone
// app. Returns the length, or 0 if `size` is too small.
size_t to_text(const Settings& s, char* out, size_t size, bool include_password);

// Reads "key=value&..." text into `s`. Unknown keys are ignored, and missing keys keep their
// value, so an older saved text still loads. Values are clamped. Returns false if the text is
// not valid.
bool from_text(const char* text, Settings& s);

// Copies the game options into a game configuration.
void apply(const Settings& s, game::GameConfig& config);

// Percent decoding of one value, for example "a%20b" to "a b". Returns false if the result does
// not fit.
bool url_decode(const char* in, size_t in_length, char* out, size_t size);

}  // namespace settings
