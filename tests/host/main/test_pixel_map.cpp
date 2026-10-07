// SPDX-License-Identifier: GPL-3.0-or-later
#include <set>

#include "life/grid.hpp"
#include "pixel_map/pixel_map.hpp"
#include "tests.hpp"
#include "unity.h"

using pixel_map::Led;
using pixel_map::PixelConfig;
using pixel_map::Start;

namespace {

struct Width {
    explicit Width(int columns) { TEST_ASSERT_TRUE(life::set_width(columns)); }
    ~Width() { life::set_width(64); }
};

void test_line_split() {
    Width width(80);  // 10 panels.
    PixelConfig config;
    TEST_ASSERT_EQUAL(10, pixel_map::panels());
    TEST_ASSERT_EQUAL(4, pixel_map::lines_used(config));
    TEST_ASSERT_EQUAL(3 * 256, pixel_map::line_length(config, 0));
    TEST_ASSERT_EQUAL(3 * 256, pixel_map::line_length(config, 1));
    TEST_ASSERT_EQUAL(2 * 256, pixel_map::line_length(config, 2));
    TEST_ASSERT_EQUAL(2 * 256, pixel_map::line_length(config, 3));
    // Panels in ring order: panel 3 is the first panel of line 1.
    const Led led = pixel_map::map_cell(config, 3 * 8, 31);
    TEST_ASSERT_EQUAL(1, led.line);
    TEST_ASSERT_EQUAL(0, led.index);
}

void test_fewer_panels_than_lines() {
    Width width(16);  // 2 panels.
    PixelConfig config;
    TEST_ASSERT_EQUAL(2, pixel_map::lines_used(config));
    TEST_ASSERT_EQUAL(0, pixel_map::line_length(config, 2));
    config.lines = 1;
    TEST_ASSERT_EQUAL(512, pixel_map::line_length(config, 0));
}

void test_default_serpentine_rows() {
    PixelConfig config;  // Bottom-left start, rows, serpentine.
    // First row from the bottom: left to right.
    TEST_ASSERT_EQUAL(0, pixel_map::map_cell(config, 0, 31).index);
    TEST_ASSERT_EQUAL(7, pixel_map::map_cell(config, 7, 31).index);
    // The second row runs back: right to left.
    TEST_ASSERT_EQUAL(8, pixel_map::map_cell(config, 7, 30).index);
    TEST_ASSERT_EQUAL(15, pixel_map::map_cell(config, 0, 30).index);
    TEST_ASSERT_EQUAL(255, pixel_map::map_cell(config, 0, 0).index);  // Top row runs back.
}

void test_columns_and_corners() {
    PixelConfig config;
    config.rows = false;
    config.start = Start::kTopRight;
    TEST_ASSERT_EQUAL(0, pixel_map::map_cell(config, 7, 0).index);
    TEST_ASSERT_EQUAL(31, pixel_map::map_cell(config, 7, 31).index);
    TEST_ASSERT_EQUAL(32, pixel_map::map_cell(config, 6, 31).index);  // Runs back up.
    config.serpentine = false;
    TEST_ASSERT_EQUAL(32, pixel_map::map_cell(config, 6, 0).index);
}

void test_every_option_is_one_to_one() {
    for (int columns : {8, 56, 80, 128}) {
        Width width(columns);
        for (int flags = 0; flags < 64; ++flags) {
            PixelConfig config;
            config.start = static_cast<Start>(flags & 3);
            config.rows = flags & 4;
            config.serpentine = flags & 8;
            config.zigzag = flags & 16;
            config.reverse_ring = flags & 32;
            config.lines = 1 + flags % 4;
            std::set<int> seen;
            for (int y = 0; y < life::kHeight; ++y) {
                for (int x = 0; x < columns; ++x) {
                    const Led led = pixel_map::map_cell(config, x, y);
                    TEST_ASSERT_TRUE(led.index < pixel_map::line_length(config, led.line));
                    TEST_ASSERT_TRUE(seen.insert(led.line * 65536 + led.index).second);
                }
            }
            TEST_ASSERT_EQUAL(columns * 32, static_cast<int>(seen.size()));
        }
    }
}

}  // namespace

void run_pixel_map_tests() {
    RUN_TEST(test_line_split);
    RUN_TEST(test_fewer_panels_than_lines);
    RUN_TEST(test_default_serpentine_rows);
    RUN_TEST(test_columns_and_corners);
    RUN_TEST(test_every_option_is_one_to_one);
}
