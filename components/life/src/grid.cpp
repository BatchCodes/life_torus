// SPDX-License-Identifier: GPL-3.0-or-later
#include "life/grid.hpp"

#include <bit>

namespace life {

uint32_t Rng::next() {
    uint32_t x = state_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state_ = x;
    return x;
}

void Grid::set(int x, int y, bool alive) {
    const uint64_t mask = uint64_t{1} << x;
    rows_[y] = alive ? (rows_[y] | mask) : (rows_[y] & ~mask);
}

void Grid::randomise(Rng& rng, int percent_alive) {
    for (int y = 0; y < kHeight; ++y) {
        uint64_t bits = 0;
        for (int x = 0; x < kWidth; ++x) {
            if (static_cast<int>(rng.below(100)) < percent_alive) {
                bits |= uint64_t{1} << x;
            }
        }
        rows_[y] = bits;
    }
}

int Grid::population() const {
    int count = 0;
    for (uint64_t bits : rows_) {
        count += std::popcount(bits);
    }
    return count;
}

bool Grid::empty() const {
    for (uint64_t bits : rows_) {
        if (bits != 0) {
            return false;
        }
    }
    return true;
}

uint32_t Grid::hash() const {
    uint32_t h = 2166136261u;
    for (uint64_t bits : rows_) {
        for (int i = 0; i < 8; ++i) {
            h ^= static_cast<uint8_t>(bits >> (i * 8));
            h *= 16777619u;
        }
    }
    return h;
}

Grid step(const Grid& grid, EdgeMode edge_mode) {
    Grid result;
    for (int y = 0; y < kHeight; ++y) {
        const bool has_up = edge_mode == EdgeMode::kTorus || y > 0;
        const bool has_down = edge_mode == EdgeMode::kTorus || y < kHeight - 1;
        const uint64_t up = has_up ? grid.row(wrap_y(y - 1)) : 0;
        const uint64_t mid = grid.row(y);
        const uint64_t down = has_down ? grid.row(wrap_y(y + 1)) : 0;

        // The ring wraps, so the left and right neighbours are rotations of the row.
        const uint64_t neighbours[8] = {
            std::rotl(up, 1),   up,   std::rotr(up, 1),   std::rotl(mid, 1), std::rotr(mid, 1),
            std::rotl(down, 1), down, std::rotr(down, 1),
        };

        // Bit-sliced count of the neighbours, modulo 8. A count of 8 reads as 0, which is
        // correct for the rule because only 2 and 3 matter.
        uint64_t s0 = 0;
        uint64_t s1 = 0;
        uint64_t s2 = 0;
        for (uint64_t n : neighbours) {
            const uint64_t c0 = s0 & n;
            s0 ^= n;
            const uint64_t c1 = s1 & c0;
            s1 ^= c0;
            s2 ^= c1;
        }
        const uint64_t two_or_three = s1 & ~s2;
        result.set_row(y, two_or_three & (s0 | mid));
    }
    return result;
}

}  // namespace life
