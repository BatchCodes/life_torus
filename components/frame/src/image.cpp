// SPDX-License-Identifier: GPL-3.0-or-later
#include "frame/image.hpp"

namespace frame {

void draw_life(Image& image, const life::Simulation& simulation, bool levels) {
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::kWidth; ++x) {
            Level level = Level::kOff;
            switch (simulation.state(x, y)) {
                case life::CellState::kDead:
                    break;
                case life::CellState::kBorn:
                    level = levels ? Level::kBright : Level::kNormal;
                    break;
                case life::CellState::kSurvives:
                    level = Level::kNormal;
                    break;
                case life::CellState::kDiesNext:
                    level = levels ? Level::kDim : Level::kNormal;
                    break;
            }
            image.set(x, y, level);
        }
    }
}

void draw_cursor(Image& image, const patterns::Shape& shape, int x, int y, life::EdgeMode edge_mode,
                 bool lit) {
    const Level level = lit ? Level::kBright : Level::kOff;
    patterns::for_each_cell(shape, x, y, edge_mode,
                            [&image, level](int cx, int cy) { image.set(cx, cy, level); });
}

void draw_wipe(Image& image, const Image& from, const Image& to, int columns) {
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::kWidth; ++x) {
            image.set(x, y, x < columns ? to.get(x, y) : from.get(x, y));
        }
    }
}

}  // namespace frame
