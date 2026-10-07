// SPDX-License-Identifier: GPL-3.0-or-later
#include "life/simulation.hpp"

namespace life {

Simulation::Simulation() {
    load(Grid{}, EdgeMode::kTorus);
}

void Simulation::load(const Grid& grid, EdgeMode edge_mode) {
    edge_mode_ = edge_mode;
    generation_ = 0;
    current_ = grid;
    previous_ = grid;
    next_ = step(current_, edge_mode_);
}

void Simulation::edit(const Grid& grid) {
    current_ = grid;
    previous_ = grid;
    next_ = step(current_, edge_mode_);
}

void Simulation::set_edge_mode(EdgeMode edge_mode) {
    edge_mode_ = edge_mode;
    next_ = step(current_, edge_mode_);
}

void Simulation::advance() {
    previous_ = current_;
    current_ = next_;
    next_ = step(current_, edge_mode_);
    ++generation_;
}

CellState Simulation::state(int x, int y) const {
    if (!current_.get(x, y)) {
        return CellState::kDead;
    }
    if (!next_.get(x, y)) {
        return CellState::kDiesNext;
    }
    if (!previous_.get(x, y)) {
        return CellState::kBorn;
    }
    return CellState::kSurvives;
}

}  // namespace life
