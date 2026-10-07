// SPDX-License-Identifier: GPL-3.0-or-later
#include "life/grid.hpp"

#include <bit>

namespace life {

namespace {

int g_width = 64;

int used_words();

// The bits of the last used word that hold cells.
uint64_t last_word_mask() {
    const int bits = g_width - (used_words() - 1) * 64;
    if (bits >= 64) {
        return ~uint64_t{0};
    }
    return bits <= 0 ? 0 : (uint64_t{1} << bits) - 1;
}

int used_words() {
    return (g_width + 63) / 64;
}

// Moves every cell one column to the right (x to x + 1), around the ring.
Grid::Row shift_right(const Grid::Row& row) {
    Grid::Row out{};
    const int words = used_words();
    const int last = g_width - 1;
    for (int w = 0; w < words; ++w) {
        out[w] = row[w] << 1;
        if (w > 0) {
            out[w] |= row[w - 1] >> 63;
        }
    }
    // The cell at the last column moves to column 0.
    out[0] |= (row[last >> 6] >> (last & 63)) & 1u;
    // Clear the cell that moved past the last column.
    const int top = last + 1;
    if ((top >> 6) < words) {
        out[top >> 6] &= ~(uint64_t{1} << (top & 63));
    }
    return out;
}

// Moves every cell one column to the left (x to x - 1), around the ring.
Grid::Row shift_left(const Grid::Row& row) {
    Grid::Row out{};
    const int words = used_words();
    const int last = g_width - 1;
    for (int w = 0; w < words; ++w) {
        out[w] = row[w] >> 1;
        if (w + 1 < words) {
            out[w] |= row[w + 1] << 63;
        }
    }
    // The cell at column 0 moves to the last column.
    if (row[0] & 1u) {
        out[last >> 6] |= uint64_t{1} << (last & 63);
    }
    return out;
}

}  // namespace

int width() {
    return g_width;
}

bool set_width(int columns) {
    if (columns < 8 || columns > kMaxWidth || columns % 8 != 0) {
        return false;
    }
    g_width = columns;
    return true;
}

uint32_t Rng::next() {
    uint32_t x = state_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state_ = x;
    return x;
}

void Grid::set(int x, int y, bool alive) {
    const uint64_t mask = uint64_t{1} << (x & 63);
    uint64_t& word = rows_[y][x >> 6];
    word = alive ? (word | mask) : (word & ~mask);
}

void Grid::clear() {
    for (Row& row : rows_) {
        row.fill(0);
    }
}

void Grid::fill_row(int y) {
    for (int x = 0; x < g_width; ++x) {
        set(x, y, true);
    }
}

void Grid::randomise(Rng& rng, int percent_alive) {
    clear();
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < g_width; ++x) {
            if (static_cast<int>(rng.below(100)) < percent_alive) {
                set(x, y, true);
            }
        }
    }
}

int Grid::population() const {
    int count = 0;
    for (const Row& row : rows_) {
        for (uint64_t bits : row) {
            count += std::popcount(bits);
        }
    }
    return count;
}

bool Grid::empty() const {
    for (const Row& row : rows_) {
        for (uint64_t bits : row) {
            if (bits != 0) {
                return false;
            }
        }
    }
    return true;
}

uint32_t Grid::hash() const {
    uint32_t h = 2166136261u;
    for (const Row& row : rows_) {
        for (uint64_t bits : row) {
            for (int i = 0; i < 8; ++i) {
                h ^= static_cast<uint8_t>(bits >> (i * 8));
                h *= 16777619u;
            }
        }
    }
    return h;
}

Grid step(const Grid& grid, EdgeMode edge_mode) {
    Grid result;
    const int words = used_words();
    const uint64_t last_mask = last_word_mask();
    for (int y = 0; y < kHeight; ++y) {
        const bool has_up = edge_mode == EdgeMode::kTorus || y > 0;
        const bool has_down = edge_mode == EdgeMode::kTorus || y < kHeight - 1;
        const Grid::Row zero{};
        const Grid::Row& up = has_up ? grid.row(wrap_y(y - 1)) : zero;
        const Grid::Row& mid = grid.row(y);
        const Grid::Row& down = has_down ? grid.row(wrap_y(y + 1)) : zero;

        // The ring wraps, so the left and right neighbours are shifted copies of the rows.
        const Grid::Row neighbours[8] = {
            shift_right(up),   up,   shift_left(up),   shift_right(mid), shift_left(mid),
            shift_right(down), down, shift_left(down),
        };

        Grid::Row next{};
        for (int w = 0; w < words; ++w) {
            // Bit-sliced count of the neighbours, modulo 8. A count of 8 reads as 0, which is
            // correct for the rule because only 2 and 3 matter.
            uint64_t s0 = 0;
            uint64_t s1 = 0;
            uint64_t s2 = 0;
            for (const Grid::Row& n : neighbours) {
                const uint64_t c0 = s0 & n[w];
                s0 ^= n[w];
                const uint64_t c1 = s1 & c0;
                s1 ^= c0;
                s2 ^= c1;
            }
            next[w] = s1 & ~s2 & (s0 | mid[w]);
        }
        next[words - 1] &= last_mask;
        result.set_row(y, next);
    }
    return result;
}

}  // namespace life
