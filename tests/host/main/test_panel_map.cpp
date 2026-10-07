// SPDX-License-Identifier: GPL-3.0-or-later
#include <set>

#include "frame/image.hpp"
#include "panel_map/panel_map.hpp"
#include "tests.hpp"
#include "unity.h"

using frame::Level;
using panel_map::Address;
using panel_map::PanelConfig;

namespace {

int key(const Address& a) {
    return (a.chip * 8 + a.digit) * 8 + a.bit;
}

void check_one_to_one(const PanelConfig& config) {
    std::set<int> seen;
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::width(); ++x) {
            const Address a = panel_map::map_cell(config, x, y);
            TEST_ASSERT_TRUE(a.chip < panel_map::chips());
            TEST_ASSERT_TRUE(a.digit < 8);
            TEST_ASSERT_TRUE(a.bit < 8);
            TEST_ASSERT_TRUE(seen.insert(key(a)).second);
        }
    }
    TEST_ASSERT_EQUAL(2048, static_cast<int>(seen.size()));
}

void test_every_layout_is_one_to_one() {
    for (int flags = 0; flags < 64; ++flags) {
        PanelConfig config;
        config.reverse_ring = flags & 1;
        config.flip_boards = flags & 2;
        config.zigzag = flags & 4;
        config.block_transpose = flags & 8;
        config.block_flip_x = flags & 16;
        config.block_flip_y = flags & 32;
        check_one_to_one(config);
    }
}

void test_default_layout() {
    const PanelConfig config;
    // Board 0, bottom block (chip 0), bottom-left cell: digit 0 (column 0), bit 7 (row 7).
    Address a = panel_map::map_cell(config, 0, 31);
    TEST_ASSERT_EQUAL(0, a.chip);
    TEST_ASSERT_EQUAL(0, a.digit);
    TEST_ASSERT_EQUAL(7, a.bit);
    // Top-right cell of board 0 is in chip 3.
    a = panel_map::map_cell(config, 7, 0);
    TEST_ASSERT_EQUAL(3, a.chip);
    TEST_ASSERT_EQUAL(7, a.digit);
    TEST_ASSERT_EQUAL(0, a.bit);
    // Column 8 is on board 1.
    TEST_ASSERT_EQUAL(4, panel_map::map_cell(config, 8, 31).chip);
    TEST_ASSERT_EQUAL(31, panel_map::map_cell(config, 63, 0).chip);
}

void test_chain_layouts() {
    PanelConfig reverse;
    reverse.reverse_ring = true;
    TEST_ASSERT_EQUAL(28, panel_map::map_cell(reverse, 0, 31).chip);
    TEST_ASSERT_EQUAL(0, panel_map::map_cell(reverse, 56, 31).chip);

    PanelConfig zigzag;
    zigzag.zigzag = true;
    // Board 1 is upside down: its chip 0 is at the top, and its columns run the other way.
    const Address a = panel_map::map_cell(zigzag, 8, 0);
    TEST_ASSERT_EQUAL(4, a.chip);
    TEST_ASSERT_EQUAL(7, a.digit);
    TEST_ASSERT_EQUAL(7, a.bit);
    TEST_ASSERT_EQUAL(0, panel_map::map_cell(zigzag, 0, 31).chip);
}

void test_subframe_levels() {
    // Three sub-frames: bright 3 of 3, normal 2 of 3, dim 1 of 3.
    int on[4] = {};
    for (int s = 0; s < 3; ++s) {
        for (int level = 0; level < 4; ++level) {
            on[level] += panel_map::cell_on(static_cast<Level>(level), s, 3, true) ? 1 : 0;
        }
    }
    TEST_ASSERT_EQUAL(0, on[0]);
    TEST_ASSERT_EQUAL(1, on[1]);
    TEST_ASSERT_EQUAL(2, on[2]);
    TEST_ASSERT_EQUAL(3, on[3]);
    // Without levels every live cell is on all the time.
    for (int s = 0; s < 3; ++s) {
        TEST_ASSERT_TRUE(panel_map::cell_on(Level::kDim, s, 3, false));
    }
}

void test_encode() {
    const PanelConfig config;
    frame::Image image;
    image.set(0, 31, Level::kBright);
    image.set(1, 31, Level::kDim);
    panel_map::Registers regs;
    panel_map::encode(config, image, 0, 3, true, regs);
    TEST_ASSERT_EQUAL_HEX8(0x80, regs[0][0]);
    TEST_ASSERT_EQUAL_HEX8(0x80, regs[1][0]);
    panel_map::encode(config, image, 2, 3, true, regs);
    TEST_ASSERT_EQUAL_HEX8(0x80, regs[0][0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, regs[1][0]);
    int total = 0;
    for (const auto& digit : regs) {
        for (uint8_t value : digit) {
            total += __builtin_popcount(value);
        }
    }
    TEST_ASSERT_EQUAL(1, total);
}

}  // namespace

void run_panel_map_tests() {
    RUN_TEST(test_every_layout_is_one_to_one);
    RUN_TEST(test_default_layout);
    RUN_TEST(test_chain_layouts);
    RUN_TEST(test_subframe_levels);
    RUN_TEST(test_encode);
}
