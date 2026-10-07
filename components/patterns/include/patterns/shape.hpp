// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>

#include "life/grid.hpp"

namespace patterns {

// A pattern of up to 64 × 32 cells. Bit x of row y is the cell at column x.
class Shape {
public:
    int width() const { return width_; }
    int height() const { return height_; }
    bool get(int x, int y) const { return (rows_[y] >> x) & 1u; }
    void set(int x, int y) { rows_[y] |= uint64_t{1} << x; }
    void resize(int width, int height);
    int population() const;

private:
    uint8_t width_ = 0;
    uint8_t height_ = 0;
    std::array<uint64_t, life::kHeight> rows_{};
};

// Parses the pattern part of a run-length encoded (RLE) pattern, for example "bo$2bo$3o!".
// Lines that start with '#' or 'x' are header lines and are skipped. Returns false if the text
// is not valid or the pattern is larger than 64 × 32.
bool parse_rle(const char* text, Shape& out);

// Turns the shape 90 degrees clockwise. A shape that is wider than the grid is high turns
// 180 degrees instead, so the result always fits on the grid.
Shape rotate(const Shape& shape);
// Mirrors the shape left to right.
Shape mirror(const Shape& shape);

// The top-left cell of a shape whose centre is at (centre_x, centre_y).
inline int origin_x(const Shape& shape, int centre_x) {
    return centre_x - shape.width() / 2;
}
inline int origin_y(const Shape& shape, int centre_y) {
    return centre_y - shape.height() / 2;
}

// Calls fn(x, y) for each live cell of the shape, centred at (centre_x, centre_y). The cells
// wrap around the ring. In torus mode they also wrap from the top to the bottom. In cylinder
// mode, cells above the top or below the bottom are skipped.
template <typename Fn>
void for_each_cell(const Shape& shape, int centre_x, int centre_y, life::EdgeMode edge_mode,
                   Fn&& fn) {
    const int left = origin_x(shape, centre_x);
    const int top = origin_y(shape, centre_y);
    for (int sy = 0; sy < shape.height(); ++sy) {
        int y = top + sy;
        if (edge_mode == life::EdgeMode::kCylinder && (y < 0 || y >= life::kHeight)) {
            continue;
        }
        y = life::wrap_y(y);
        for (int sx = 0; sx < shape.width(); ++sx) {
            if (shape.get(sx, sy)) {
                fn(life::wrap_x(left + sx), y);
            }
        }
    }
}

// Sets the live cells of the shape on the grid.
void stamp(life::Grid& grid, const Shape& shape, int centre_x, int centre_y,
           life::EdgeMode edge_mode);
// Clears the grid cells under the live cells of the shape.
void erase(life::Grid& grid, const Shape& shape, int centre_x, int centre_y,
           life::EdgeMode edge_mode);

}  // namespace patterns
