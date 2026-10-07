// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

#include "life/grid.hpp"

namespace life {

enum class CellState : uint8_t {
    kDead,
    kBorn,      // Alive now, dead in the previous generation.
    kSurvives,  // Alive now and in the previous generation, and alive in the next generation.
    kDiesNext,  // Alive now, dead in the next generation.
};

// The board with one generation of history and one generation of look-ahead, so a renderer
// can show new cells and cells that are about to die.
class Simulation {
public:
    Simulation();

    // Replaces the board. The new cells count as existing cells, not as born cells.
    void load(const Grid& grid, EdgeMode edge_mode);
    // Applies an edit (for example a stamp) to the current board. Edited cells do not count as
    // born.
    void edit(const Grid& grid);
    void set_edge_mode(EdgeMode edge_mode);

    void advance();

    const Grid& current() const { return current_; }
    const Grid& next() const { return next_; }
    EdgeMode edge_mode() const { return edge_mode_; }
    uint32_t generation() const { return generation_; }

    CellState state(int x, int y) const;

private:
    Grid previous_;
    Grid current_;
    Grid next_;
    EdgeMode edge_mode_ = EdgeMode::kTorus;
    uint32_t generation_ = 0;
};

}  // namespace life
