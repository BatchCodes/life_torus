// SPDX-License-Identifier: GPL-3.0-or-later
#include "frame/image.hpp"
#include "life/simulation.hpp"
#include "patterns/library.hpp"
#include "tests.hpp"
#include "unity.h"

using frame::Image;
using frame::Level;

namespace {

int count(const Image& image, Level level) {
    int n = 0;
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::kWidth; ++x) {
            if (image.get(x, y) == level) {
                ++n;
            }
        }
    }
    return n;
}

life::Simulation blinker_after_one_step() {
    life::Grid grid;
    grid.set(10, 10, true);
    grid.set(11, 10, true);
    grid.set(12, 10, true);
    life::Simulation sim;
    sim.load(grid, life::EdgeMode::kTorus);
    sim.advance();
    return sim;
}

void test_life_levels() {
    // A vertical blinker: the top and bottom cells are born and die next, the centre survives.
    const life::Simulation sim = blinker_after_one_step();
    Image image;
    frame::draw_life(image, sim, true);
    TEST_ASSERT_EQUAL(static_cast<int>(Level::kNormal), static_cast<int>(image.get(11, 10)));
    TEST_ASSERT_EQUAL(static_cast<int>(Level::kDim), static_cast<int>(image.get(11, 9)));
    TEST_ASSERT_EQUAL(static_cast<int>(Level::kDim), static_cast<int>(image.get(11, 11)));
    TEST_ASSERT_EQUAL(life::kWidth * life::kHeight - 3, count(image, Level::kOff));

    // A glider has born cells that survive into the next generation.
    life::Grid glider;
    glider.set(21, 20, true);
    glider.set(22, 21, true);
    glider.set(20, 22, true);
    glider.set(21, 22, true);
    glider.set(22, 22, true);
    life::Simulation sim2;
    sim2.load(glider, life::EdgeMode::kTorus);
    sim2.advance();
    frame::draw_life(image, sim2, true);
    TEST_ASSERT_TRUE(count(image, Level::kBright) > 0);
}

void test_life_without_levels() {
    const life::Simulation sim = blinker_after_one_step();
    Image image;
    frame::draw_life(image, sim, false);
    TEST_ASSERT_EQUAL(3, count(image, Level::kNormal));
    TEST_ASSERT_EQUAL(0, count(image, Level::kDim));
    TEST_ASSERT_EQUAL(0, count(image, Level::kBright));
}

void test_cursor_blinks_on_live_cell() {
    const patterns::Shape cell = patterns::load_shape(0);
    Image image;
    image.set(5, 5, Level::kNormal);
    frame::draw_cursor(image, cell, 5, 5, life::EdgeMode::kTorus, true);
    TEST_ASSERT_EQUAL(static_cast<int>(Level::kBright), static_cast<int>(image.get(5, 5)));
    frame::draw_cursor(image, cell, 5, 5, life::EdgeMode::kTorus, false);
    TEST_ASSERT_EQUAL(static_cast<int>(Level::kOff), static_cast<int>(image.get(5, 5)));
}

void test_wipe() {
    Image from;
    Image to;
    from.fill(Level::kNormal);
    to.fill(Level::kOff);
    Image image;
    frame::draw_wipe(image, from, to, 16);
    TEST_ASSERT_EQUAL(static_cast<int>(Level::kOff), static_cast<int>(image.get(15, 3)));
    TEST_ASSERT_EQUAL(static_cast<int>(Level::kNormal), static_cast<int>(image.get(16, 3)));
    frame::draw_wipe(image, from, to, life::kWidth);
    TEST_ASSERT_TRUE(image == to);
}

void test_ko_effect_moves_around_the_ring() {
    Image a;
    Image b;
    frame::draw_ko_code(a, 0);
    frame::draw_ko_code(b, 1);
    TEST_ASSERT_TRUE(count(a, Level::kBright) > 100);
    // Rows outside the text band stay dark.
    for (int x = 0; x < life::kWidth; ++x) {
        TEST_ASSERT_EQUAL(static_cast<int>(Level::kOff), static_cast<int>(a.get(x, 0)));
        TEST_ASSERT_EQUAL(static_cast<int>(Level::kOff), static_cast<int>(a.get(x, 31)));
    }
    // One column of movement: column x of offset 1 is column x + 1 of offset 0.
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::kWidth - 1; ++x) {
            TEST_ASSERT_EQUAL(static_cast<int>(a.get(x + 1, y)), static_cast<int>(b.get(x, y)));
        }
    }
    // The text crosses the seam: some offset lights both column 63 and column 0.
    bool crossed = false;
    for (int offset = 0; offset < 200 && !crossed; ++offset) {
        frame::draw_ko_code(a, offset);
        for (int y = 0; y < life::kHeight; ++y) {
            if (a.get(63, y) == Level::kBright && a.get(0, y) == Level::kBright) {
                crossed = true;
            }
        }
    }
    TEST_ASSERT_TRUE(crossed);
}

}  // namespace

void run_frame_tests() {
    RUN_TEST(test_life_levels);
    RUN_TEST(test_life_without_levels);
    RUN_TEST(test_cursor_blinks_on_live_cell);
    RUN_TEST(test_wipe);
    RUN_TEST(test_ko_effect_moves_around_the_ring);
}
