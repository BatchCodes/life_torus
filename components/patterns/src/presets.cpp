// SPDX-License-Identifier: GPL-3.0-or-later
//
// The preset boards. tools/preset_survey measures how long each one stays interesting.
// docs/software/presets.md has the results.
#include "patterns/presets.hpp"

#include "patterns/library.hpp"
#include "patterns/shape.hpp"

namespace patterns {

namespace {

using life::EdgeMode;

constexpr Placement kGosperGun[] = {
    {"Gosper glider gun", 14, 3, 0, false},
};

constexpr Placement kRPentomino[] = {
    {"R-pentomino", 30, 14, 0, false},
};

constexpr Placement kAcorn[] = {
    {"Acorn", 28, 15, 0, false},
};

constexpr Placement kFleet[] = {
    {"Lightweight spaceship", 4, 3, 0, false},   {"Middleweight spaceship", 40, 2, 0, true},
    {"Heavyweight spaceship", 10, 11, 0, false}, {"Lightweight spaceship", 26, 20, 0, false},
    {"Middleweight spaceship", 52, 21, 0, true},
};

constexpr Placement kGarden[] = {
    {"Pulsar", 2, 2, 0, false},          {"Pentadecathlon", 22, 6, 1, false},
    {"Beacon", 36, 3, 0, false},         {"Toad", 44, 4, 0, false},
    {"Blinker", 54, 4, 0, false},        {"Beacon", 36, 22, 0, true},
    {"Toad", 46, 24, 0, false},          {"Blinker", 56, 22, 1, false},
    {"Pentadecathlon", 4, 24, 0, false},
};

constexpr Placement kDiehard[] = {
    {"Diehard", 28, 15, 0, false},
};

constexpr Placement kGliderSwarm[] = {
    {"Glider", 4, 4, 0, false},  {"Glider", 20, 6, 1, false}, {"Glider", 36, 3, 2, false},
    {"Glider", 52, 8, 3, false}, {"Glider", 10, 20, 0, true}, {"Glider", 26, 24, 1, true},
    {"Glider", 42, 18, 2, true}, {"Glider", 56, 26, 3, true},
};

constexpr Placement kPiHeptomino[] = {
    {"Pi-heptomino", 31, 14, 0, false},
};

constexpr Placement kSimkinGun[] = {
    {"Simkin glider gun", 16, 5, 0, false},
};

constexpr Preset kPresets[] = {
    {"Glider gun", PresetKind::kComposed, EdgeMode::kTorus, kGosperGun},
    {"R-pentomino", PresetKind::kComposed, EdgeMode::kTorus, kRPentomino},
    {"Spaceship fleet", PresetKind::kComposed, EdgeMode::kTorus, kFleet},
    {"Acorn", PresetKind::kComposed, EdgeMode::kTorus, kAcorn},
    {"Oscillator garden", PresetKind::kComposed, EdgeMode::kTorus, kGarden},
    {"Glider swarm", PresetKind::kComposed, EdgeMode::kTorus, kGliderSwarm},
    {"Diehard", PresetKind::kComposed, EdgeMode::kTorus, kDiehard},
    {"Pi-heptomino", PresetKind::kComposed, EdgeMode::kTorus, kPiHeptomino},
    {"Simkin gun", PresetKind::kComposed, EdgeMode::kCylinder, kSimkinGun},
    {"Random soup", PresetKind::kRandom, EdgeMode::kTorus, {}},
    {"Empty board", PresetKind::kEmpty, EdgeMode::kTorus, {}},
};

}  // namespace

std::span<const Preset> presets() {
    return kPresets;
}

bool preset_fits(const Preset& preset) {
    for (const Placement& placement : preset.placements) {
        const int index = find_shape(placement.shape);
        if (index < 0) {
            return false;
        }
        Shape shape = load_shape(index);
        for (int i = 0; i < placement.turns; ++i) {
            shape = rotate(shape);
        }
        if (shape.width() > life::width()) {
            return false;
        }
    }
    return true;
}

EdgeMode build_preset(const Preset& preset, life::Grid& grid, life::Rng& rng,
                      EdgeMode default_edge_mode, int percent_alive) {
    grid.clear();
    switch (preset.kind) {
        case PresetKind::kEmpty:
            return default_edge_mode;
        case PresetKind::kRandom:
            grid.randomise(rng, percent_alive);
            return default_edge_mode;
        case PresetKind::kComposed:
            break;
    }
    for (const Placement& placement : preset.placements) {
        const int index = find_shape(placement.shape);
        if (index < 0) {
            continue;
        }
        Shape shape = load_shape(index);
        for (int i = 0; i < placement.turns; ++i) {
            shape = rotate(shape);
        }
        if (placement.mirrored) {
            shape = mirror(shape);
        }
        // The positions are for 64 columns. stamp() takes the centre of the shape.
        const int x = placement.x * life::width() / 64;
        stamp(grid, shape, x + shape.width() / 2, placement.y + shape.height() / 2,
              preset.edge_mode);
    }
    return preset.edge_mode;
}

}  // namespace patterns
