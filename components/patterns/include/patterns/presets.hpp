// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <span>

#include "life/grid.hpp"

namespace patterns {

// One shape from the cursor shape library on a preset board. (x, y) is the top-left cell. The
// shape turns first, then mirrors.
struct Placement {
    const char* shape;
    int x;
    int y;
    uint8_t turns;  // Number of 90 degree turns clockwise.
    bool mirrored;
};

enum class PresetKind : uint8_t { kComposed, kEmpty, kRandom };

struct Preset {
    const char* name;
    PresetKind kind;
    // The edge mode of a composed preset. Empty and random presets use the configured edge mode.
    life::EdgeMode edge_mode;
    std::span<const Placement> placements;
};

// The preset boards, in the order that Select steps through them.
std::span<const Preset> presets();

// Builds the board of a preset. A random preset uses the generator and percent_alive.
// Returns the edge mode of the board.
life::EdgeMode build_preset(const Preset& preset, life::Grid& grid, life::Rng& rng,
                            life::EdgeMode default_edge_mode, int percent_alive);

}  // namespace patterns
