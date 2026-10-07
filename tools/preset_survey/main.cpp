// SPDX-License-Identifier: GPL-3.0-or-later
//
// Runs each preset board on the 64 × 32 grid and prints a Markdown table for
// docs/software/presets.md. The limits are the firmware defaults.
#include <cstdio>
#include <unordered_map>

#include "life/grid.hpp"
#include "patterns/presets.hpp"

namespace {

constexpr int kMaxGenerations = 20000;
constexpr int kSettledLimit = 30;
constexpr int kRepeatWindow = 32;
constexpr int kRepeatLimit = 300;
constexpr int kNoInputLimit = 10000;
constexpr uint32_t kSeed = 1;
constexpr int kPercentAlive = 30;

struct Result {
    int start_population = 0;
    int end_generation = -1;  // First generation of the final cycle, or the empty generation.
    int period = 0;           // 0 when the board is empty or the run reached the maximum.
    bool empty = false;
    int on_display = 0;  // Generations until an unattended-play rule loads a new preset.
};

Result survey(const patterns::Preset& preset) {
    life::Grid grid;
    life::Rng rng(kSeed);
    const life::EdgeMode mode =
        patterns::build_preset(preset, grid, rng, life::EdgeMode::kTorus, kPercentAlive);
    Result result;
    result.start_population = grid.population();

    std::unordered_map<uint32_t, int> seen;
    for (int generation = 0; generation <= kMaxGenerations; ++generation) {
        if (grid.empty()) {
            result.empty = true;
            result.end_generation = generation;
            break;
        }
        const auto [it, inserted] = seen.emplace(grid.hash(), generation);
        if (!inserted) {
            result.end_generation = it->second;
            result.period = generation - it->second;
            break;
        }
        grid = life::step(grid, mode);
    }

    if (result.empty || result.period == 1 || result.period == 2) {
        const int period = result.empty ? 1 : result.period;
        result.on_display = result.end_generation + period + kSettledLimit - 1;
    } else if (result.period > 0 && result.period <= kRepeatWindow) {
        result.on_display = result.end_generation + result.period + kRepeatLimit;
    } else {
        result.on_display = kNoInputLimit;
    }
    if (result.on_display > kNoInputLimit) {
        result.on_display = kNoInputLimit;
    }
    return result;
}

const char* edge_name(const patterns::Preset& preset) {
    if (preset.kind != patterns::PresetKind::kComposed) {
        return "configured (torus)";
    }
    return preset.edge_mode == life::EdgeMode::kTorus ? "torus" : "cylinder";
}

}  // namespace

int main() {
    std::printf("| Preset | Edges | Cells | Settles at | End state | On display |\n");
    std::printf("| --- | --- | --- | --- | --- | --- |\n");
    for (const patterns::Preset& preset : patterns::presets()) {
        const Result r = survey(preset);
        char end_state[48];
        if (preset.kind == patterns::PresetKind::kEmpty) {
            std::snprintf(end_state, sizeof(end_state), "empty (for drawing)");
        } else if (r.empty) {
            std::snprintf(end_state, sizeof(end_state), "empty");
        } else if (r.period == 0) {
            std::snprintf(end_state, sizeof(end_state), "still active");
        } else if (r.period == 1) {
            std::snprintf(end_state, sizeof(end_state), "still life");
        } else {
            std::snprintf(end_state, sizeof(end_state), "period %d", r.period);
        }
        char settles[24];
        if (r.end_generation < 0) {
            std::snprintf(settles, sizeof(settles), "> %d", kMaxGenerations);
        } else {
            std::snprintf(settles, sizeof(settles), "%d", r.end_generation);
        }
        std::printf("| %s | %s | %d | %s | %s | %d |\n", preset.name, edge_name(preset),
                    r.start_population, settles, end_state, r.on_display);
    }
    return 0;
}
