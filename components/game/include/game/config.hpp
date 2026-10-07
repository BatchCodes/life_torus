// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

#include "life/grid.hpp"

namespace game {

// All timings and limits of the game. The defaults are the firmware defaults. The firmware
// fills this struct from Kconfig, and the simulator from its settings panel.
struct GameConfig {
    uint32_t step_ms = 100;         // Time between generations in run.
    bool brightness_levels = true;  // Born cells bright, dying cells dim.
    // Generations with a settled board (empty, still, or period 2) before a new preset.
    uint32_t settled_limit = 30;
    uint32_t no_input_limit = 10000;  // Generations with no button press before a new preset.
    uint32_t repeat_limit = 300;      // Generations of a short repeat before a new preset. 0: off.
    uint32_t pause_timeout_ms = 30000;  // Pause with no button press, then run.
    uint32_t transition_ms = 800;       // Wipe from the old board to a new preset.
    uint32_t cursor_blink_ms = 250;     // Half of the cursor blink period.
    uint32_t repeat_delay_ms = 300;     // D-pad held: time before the first repeat.
    uint32_t repeat_interval_ms = 80;   // D-pad held: time between repeats.
    uint32_t ko_effect_ms = 10000;      // Length of the ko code effect.
    uint32_t ko_gap_ms = 2000;          // Longest time between two ko code presses.
    uint32_t ko_column_ms = 40;         // Ko code effect: time to move one column.
    life::EdgeMode default_edge_mode = life::EdgeMode::kTorus;  // Empty and random presets.
    int random_percent = 30;                                    // Live cells in a random preset.
    uint32_t seed = 1;  // Random generator seed. The firmware uses a random seed.
};

}  // namespace game
