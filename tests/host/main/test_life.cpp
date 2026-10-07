// SPDX-License-Identifier: GPL-3.0-or-later
#include <chrono>
#include <cstdio>

#include "life/grid.hpp"
#include "life/simulation.hpp"
#include "tests.hpp"
#include "unity.h"

using life::CellState;
using life::EdgeMode;
using life::Grid;
using life::Simulation;

namespace {

// Places cells from rows of text. 'O' is alive, any other character is dead.
Grid make(int left, int top, std::initializer_list<const char*> rows) {
    Grid grid;
    int y = top;
    for (const char* row : rows) {
        for (int x = 0; row[x] != '\0'; ++x) {
            if (row[x] == 'O') {
                grid.set(life::wrap_x(left + x), life::wrap_y(y), true);
            }
        }
        ++y;
    }
    return grid;
}

Grid run(Grid grid, EdgeMode mode, int generations) {
    for (int i = 0; i < generations; ++i) {
        grid = life::step(grid, mode);
    }
    return grid;
}

void test_block_is_still() {
    const Grid block = make(10, 10, {"OO", "OO"});
    TEST_ASSERT_TRUE(life::step(block, EdgeMode::kTorus) == block);
}

void test_beehive_is_still() {
    const Grid hive = make(20, 5, {".OO.", "O..O", ".OO."});
    TEST_ASSERT_TRUE(life::step(hive, EdgeMode::kTorus) == hive);
}

void test_blinker_has_period_two() {
    const Grid blinker = make(30, 15, {"OOO"});
    const Grid next = life::step(blinker, EdgeMode::kTorus);
    TEST_ASSERT_TRUE(next != blinker);
    TEST_ASSERT_TRUE(next == make(31, 14, {"O", "O", "O"}));
    TEST_ASSERT_TRUE(life::step(next, EdgeMode::kTorus) == blinker);
}

void test_blinker_across_seam() {
    // Cells at x = 63, 0 and 1: the blinker sits on the seam of the ring.
    const Grid blinker = make(63, 15, {"OOO"});
    TEST_ASSERT_EQUAL(3, blinker.population());
    const Grid next = life::step(blinker, EdgeMode::kCylinder);
    TEST_ASSERT_TRUE(next == make(0, 14, {"O", "O", "O"}));
}

void test_glider_moves_one_cell_in_four_generations() {
    const Grid glider = make(5, 5, {".O.", "..O", "OOO"});
    TEST_ASSERT_TRUE(run(glider, EdgeMode::kTorus, 4) == make(6, 6, {".O.", "..O", "OOO"}));
}

void test_glider_wraps_around_torus() {
    // The glider moves (+1, +1) every 4 generations. It crosses the seam (x 63 to 0) and the
    // top-bottom edge, and comes back after 4 × 64 generations.
    const Grid glider = make(60, 28, {".O.", "..O", "OOO"});
    TEST_ASSERT_TRUE(run(glider, EdgeMode::kTorus, 4 * 6) == make(2, 2, {".O.", "..O", "OOO"}));
    TEST_ASSERT_TRUE(run(glider, EdgeMode::kTorus, 4 * life::width()) == glider);
}

void test_glider_stops_at_cylinder_edge() {
    const Grid glider = make(60, 20, {".O.", "..O", "OOO"});
    // In cylinder mode the glider crosses the seam but not the bottom edge.
    TEST_ASSERT_TRUE(run(glider, EdgeMode::kCylinder, 4 * 6) == make(2, 26, {".O.", "..O", "OOO"}));
    const Grid end = run(glider, EdgeMode::kCylinder, 200);
    TEST_ASSERT_TRUE(life::step(end, EdgeMode::kCylinder) == end);
    TEST_ASSERT_TRUE(end != run(glider, EdgeMode::kTorus, 200));
}

void test_full_neighbourhood_dies() {
    Grid grid;
    for (int y = 0; y < life::kHeight; ++y) {
        grid.fill_row(y);
    }
    TEST_ASSERT_TRUE(life::step(grid, EdgeMode::kTorus).empty());
}

void test_hash_changes_with_cells() {
    Grid a;
    Grid b;
    TEST_ASSERT_EQUAL_UINT32(a.hash(), b.hash());
    b.set(3, 4, true);
    TEST_ASSERT_NOT_EQUAL(a.hash(), b.hash());
}

void test_random_fill_is_repeatable() {
    life::Rng rng_a(42);
    life::Rng rng_b(42);
    Grid a;
    Grid b;
    a.randomise(rng_a, 30);
    b.randomise(rng_b, 30);
    TEST_ASSERT_TRUE(a == b);
    TEST_ASSERT_INT_WITHIN(150, 2048 * 30 / 100, a.population());
}

void test_cell_states() {
    Simulation sim;
    sim.load(make(30, 15, {"OOO"}), EdgeMode::kTorus);
    // After load, no cell counts as born. The ends of the blinker die next.
    TEST_ASSERT_EQUAL(static_cast<int>(CellState::kDiesNext), static_cast<int>(sim.state(30, 15)));
    TEST_ASSERT_EQUAL(static_cast<int>(CellState::kSurvives), static_cast<int>(sim.state(31, 15)));
    TEST_ASSERT_EQUAL(static_cast<int>(CellState::kDead), static_cast<int>(sim.state(31, 14)));
    sim.advance();
    TEST_ASSERT_EQUAL_UINT32(1, sim.generation());
    // The new vertical cells are born, and they die in the next generation.
    TEST_ASSERT_EQUAL(static_cast<int>(CellState::kDiesNext), static_cast<int>(sim.state(31, 14)));
    TEST_ASSERT_EQUAL(static_cast<int>(CellState::kDiesNext), static_cast<int>(sim.state(31, 16)));

    sim.load(make(10, 10, {".O.", "..O", "OOO"}), EdgeMode::kTorus);
    sim.advance();
    int born = 0;
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::width(); ++x) {
            if (sim.state(x, y) == CellState::kBorn) {
                ++born;
            }
        }
    }
    TEST_ASSERT_TRUE(born > 0);
}

void test_step_time() {
    life::Rng rng(7);
    Grid grid;
    grid.randomise(rng, 35);
    constexpr int kSteps = 10000;
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kSteps; ++i) {
        grid = life::step(grid, EdgeMode::kTorus);
    }
    const auto end = std::chrono::steady_clock::now();
    const double us = std::chrono::duration<double, std::micro>(end - start).count() / kSteps;
    std::printf("life::step: %.2f us per generation on this computer\n", us);
    TEST_ASSERT_TRUE(us < 1000.0);
}

// The rule, cell by cell, for a check of the bit-sliced step.
Grid slow_step(const Grid& grid, EdgeMode mode) {
    Grid out;
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::width(); ++x) {
            int n = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) {
                        continue;
                    }
                    const int yy = y + dy;
                    if (mode == EdgeMode::kCylinder && (yy < 0 || yy >= life::kHeight)) {
                        continue;
                    }
                    n += grid.get(life::wrap_x(x + dx), life::wrap_y(yy)) ? 1 : 0;
                }
            }
            out.set(x, y, n == 3 || (n == 2 && grid.get(x, y)));
        }
    }
    return out;
}

// Runs a test at each width, then sets the width back to 64.
void test_all_widths() {
    for (int width : {8, 32, 56, 64, 72, 80, 128}) {
        TEST_ASSERT_TRUE(life::set_width(width));
        life::Rng rng(static_cast<uint32_t>(width));
        for (EdgeMode mode : {EdgeMode::kTorus, EdgeMode::kCylinder}) {
            Grid grid;
            grid.randomise(rng, 35);
            for (int i = 0; i < 20; ++i) {
                const Grid fast = life::step(grid, mode);
                TEST_ASSERT_TRUE_MESSAGE(fast == slow_step(grid, mode), "step differs");
                grid = fast;
            }
            // No cell past the width.
            for (int y = 0; y < life::kHeight; ++y) {
                for (int x = width; x < life::kMaxWidth; ++x) {
                    TEST_ASSERT_FALSE(grid.get(x, y));
                }
            }
        }
        // A glider comes back after it has crossed the seam and the top-bottom edge.
        const Grid glider = make(width - 3, 28, {".O.", "..O", "OOO"});
        int period = width;
        while (period % life::kHeight != 0) {
            period += width;
        }
        TEST_ASSERT_TRUE(run(glider, EdgeMode::kTorus, 4 * period) == glider);
    }
    TEST_ASSERT_FALSE(life::set_width(12));
    TEST_ASSERT_FALSE(life::set_width(136));
    TEST_ASSERT_TRUE(life::set_width(64));
}

}  // namespace

void run_life_tests() {
    RUN_TEST(test_all_widths);
    RUN_TEST(test_block_is_still);
    RUN_TEST(test_beehive_is_still);
    RUN_TEST(test_blinker_has_period_two);
    RUN_TEST(test_blinker_across_seam);
    RUN_TEST(test_glider_moves_one_cell_in_four_generations);
    RUN_TEST(test_glider_wraps_around_torus);
    RUN_TEST(test_glider_stops_at_cylinder_edge);
    RUN_TEST(test_full_neighbourhood_dies);
    RUN_TEST(test_hash_changes_with_cells);
    RUN_TEST(test_random_fill_is_repeatable);
    RUN_TEST(test_cell_states);
    RUN_TEST(test_step_time);
}
