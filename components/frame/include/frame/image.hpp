// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>

#include "life/grid.hpp"
#include "life/simulation.hpp"
#include "patterns/shape.hpp"

namespace frame {

// The brightness of one cell on the display. Off is dark.
enum class Level : uint8_t { kOff = 0, kDim = 1, kNormal = 2, kBright = 3 };

// One display image of 64 × 32 cells, before the conversion to the MAX7219 registers.
class Image {
public:
    Level get(int x, int y) const { return cells_[y * life::kWidth + x]; }
    void set(int x, int y, Level level) { cells_[y * life::kWidth + x] = level; }
    void fill(Level level) { cells_.fill(level); }

    bool operator==(const Image& other) const { return cells_ == other.cells_; }

private:
    std::array<Level, life::kWidth * life::kHeight> cells_{};
};

// Draws the Life layer. With levels, a born cell is bright, a surviving cell is normal and a
// cell that dies in the next generation is dim. Without levels, each live cell is normal.
void draw_life(Image& image, const life::Simulation& simulation, bool levels);

// Draws the cursor shape centred at (x, y). A lit cursor is bright. An unlit cursor is off, so
// the cursor blinks on live cells and on dead cells.
void draw_cursor(Image& image, const patterns::Shape& shape, int x, int y, life::EdgeMode edge_mode,
                 bool lit);

// Shows `to` in the first `columns` columns around the ring and `from` in the other columns.
void draw_wipe(Image& image, const Image& from, const Image& to, int columns);

// Draws the ko code effect, moved `offset` columns around the ring.
void draw_ko_code(Image& image, int offset);

}  // namespace frame
