// SPDX-License-Identifier: GPL-3.0-or-later
#include "patterns/shape.hpp"

#include <bit>

namespace patterns {

void Shape::resize(int width, int height) {
    width_ = static_cast<uint8_t>(width);
    height_ = static_cast<uint8_t>(height);
    rows_.fill(0);
}

int Shape::population() const {
    int count = 0;
    for (int y = 0; y < height_; ++y) {
        count += std::popcount(rows_[y]);
    }
    return count;
}

bool parse_rle(const char* text, Shape& out) {
    // First pass: skip the header lines and find the size. Second pass: set the cells.
    const char* body = text;
    while (*body == '#' || *body == 'x') {
        while (*body != '\0' && *body != '\n') {
            ++body;
        }
        if (*body == '\n') {
            ++body;
        }
    }

    for (int pass = 0; pass < 2; ++pass) {
        int x = 0;
        int y = 0;
        int width = 0;
        int count = 0;
        bool ended = false;
        for (const char* p = body; *p != '\0' && !ended; ++p) {
            const char c = *p;
            if (c >= '0' && c <= '9') {
                count = count * 10 + (c - '0');
                if (count > 1000) {
                    return false;
                }
                continue;
            }
            const int run = count == 0 ? 1 : count;
            count = 0;
            switch (c) {
                case 'b':
                case '.':
                    x += run;
                    break;
                case 'o':
                case 'A':
                    if (pass == 1) {
                        for (int i = 0; i < run; ++i) {
                            out.set(x + i, y);
                        }
                    }
                    x += run;
                    break;
                case '$':
                    y += run;
                    x = 0;
                    break;
                case '!':
                    ended = true;
                    break;
                case ' ':
                case '\n':
                case '\r':
                case '\t':
                    break;
                default:
                    return false;
            }
            if (x > width) {
                width = x;
            }
            if (width > kMaxShapeWidth || y >= life::kHeight) {
                return false;
            }
        }
        if (pass == 0) {
            if (!ended || width == 0) {
                return false;
            }
            out.resize(width, y + 1);
        }
    }
    return true;
}

Shape rotate(const Shape& shape) {
    Shape result;
    if (shape.width() <= life::kHeight) {
        result.resize(shape.height(), shape.width());
        for (int y = 0; y < shape.height(); ++y) {
            for (int x = 0; x < shape.width(); ++x) {
                if (shape.get(x, y)) {
                    result.set(shape.height() - 1 - y, x);
                }
            }
        }
        return result;
    }
    result.resize(shape.width(), shape.height());
    for (int y = 0; y < shape.height(); ++y) {
        for (int x = 0; x < shape.width(); ++x) {
            if (shape.get(x, y)) {
                result.set(shape.width() - 1 - x, shape.height() - 1 - y);
            }
        }
    }
    return result;
}

Shape mirror(const Shape& shape) {
    Shape result;
    result.resize(shape.width(), shape.height());
    for (int y = 0; y < shape.height(); ++y) {
        for (int x = 0; x < shape.width(); ++x) {
            if (shape.get(x, y)) {
                result.set(shape.width() - 1 - x, y);
            }
        }
    }
    return result;
}

void stamp(life::Grid& grid, const Shape& shape, int centre_x, int centre_y,
           life::EdgeMode edge_mode) {
    for_each_cell(shape, centre_x, centre_y, edge_mode,
                  [&grid](int x, int y) { grid.set(x, y, true); });
}

void erase(life::Grid& grid, const Shape& shape, int centre_x, int centre_y,
           life::EdgeMode edge_mode) {
    for_each_cell(shape, centre_x, centre_y, edge_mode,
                  [&grid](int x, int y) { grid.set(x, y, false); });
}

}  // namespace patterns
