// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>

namespace life {

// The width is 8 columns per board, 1 to 16 boards. Set it once at start-up, before any grid
// is used. The default is 8 boards (64 columns).
constexpr int kMaxWidth = 128;
constexpr int kHeight = 32;
constexpr int kWords = kMaxWidth / 64;

int width();
// Sets the width in columns (a multiple of 8, 8 to 128). Returns false and keeps the old width
// for a value outside that range.
bool set_width(int columns);

// The grid always wraps around the ring (x). Torus also wraps from the top to the bottom (y).
// Cylinder has dead cells above the top row and below the bottom row.
enum class EdgeMode : uint8_t { kTorus, kCylinder };

inline int wrap_x(int x) {
    return ((x % width()) + width()) % width();
}
constexpr int wrap_y(int y) {
    return ((y % kHeight) + kHeight) % kHeight;
}

// Small, fast pseudo-random generator (xorshift32). The same seed gives the same boards on the
// device, in the host tests and in the simulator.
class Rng {
public:
    explicit Rng(uint32_t seed = 1) : state_(seed == 0 ? 1 : seed) {}
    uint32_t next();
    // A value in [0, bound). bound must be greater than 0.
    uint32_t below(uint32_t bound) { return next() % bound; }

private:
    uint32_t state_;
};

// width() × 32 cells. Bit x % 64 of word x / 64 of row y is the cell at column x.
class Grid {
public:
    using Row = std::array<uint64_t, kWords>;

    bool get(int x, int y) const { return (rows_[y][x >> 6] >> (x & 63)) & 1u; }
    void set(int x, int y, bool alive);
    void toggle(int x, int y) { rows_[y][x >> 6] ^= uint64_t{1} << (x & 63); }
    void clear();
    void randomise(Rng& rng, int percent_alive);
    // Sets every cell of a row.
    void fill_row(int y);

    const Row& row(int y) const { return rows_[y]; }
    void set_row(int y, const Row& bits) { rows_[y] = bits; }

    int population() const;
    bool empty() const;
    // A 32-bit hash of the cells (FNV-1a over the rows), for repeat detection.
    uint32_t hash() const;

    bool operator==(const Grid& other) const { return rows_ == other.rows_; }
    bool operator!=(const Grid& other) const { return rows_ != other.rows_; }

private:
    std::array<Row, kHeight> rows_{};
};

// The next generation under the B3/S23 rule.
Grid step(const Grid& grid, EdgeMode edge_mode);

}  // namespace life
