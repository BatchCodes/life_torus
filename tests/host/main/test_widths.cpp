// SPDX-License-Identifier: GPL-3.0-or-later
//
// The board count: each component at other widths than the default 64 columns.
#include <cstring>
#include <set>

#include "display_modes/mode_manager.hpp"
#include "game/game.hpp"
#include "panel_map/panel_map.hpp"
#include "patterns/library.hpp"
#include "patterns/presets.hpp"
#include "phone_link/protocol.hpp"
#include "tests.hpp"
#include "unity.h"

namespace {

// Sets a width for one test and puts 64 back at the end.
struct Width {
    explicit Width(int columns) { TEST_ASSERT_TRUE(life::set_width(columns)); }
    ~Width() { life::set_width(64); }
};

void test_narrow_ring_skips_wide_shapes_and_presets() {
    Width width(32);
    game::Game game{game::GameConfig{}};
    uint32_t now = 0;
    game.start(now);
    for (int i = 0; i < 100; ++i) {
        game.tick(now += 10);
    }
    game.press(game::Button::kStart, now += 20);  // Pause.
    for (int i = 0; i < 40; ++i) {
        game.press(game::Button::kR, now += 20);
        TEST_ASSERT_TRUE(patterns::shape_fits(game.shape_index()));
        TEST_ASSERT_TRUE(std::strstr(game.shape_name(), "gun") == nullptr);
    }
    for (int i = 0; i < 30; ++i) {
        game.press(game::Button::kSelect, now += 20);
        TEST_ASSERT_TRUE(patterns::preset_fits(patterns::presets()[game.preset_index()]));
        TEST_ASSERT_TRUE(std::strstr(game.preset_name(), "gun") == nullptr);
    }
}

void test_wide_ring_fits_everything() {
    Width width(128);
    for (const auto& preset : patterns::presets()) {
        TEST_ASSERT_TRUE(patterns::preset_fits(preset));
    }
    for (int i = 0; i < static_cast<int>(patterns::cursor_shapes().size()); ++i) {
        TEST_ASSERT_TRUE(patterns::shape_fits(i));
    }
}

void test_panel_map_at_each_width() {
    for (int columns : {8, 32, 80, 128}) {
        Width width(columns);
        TEST_ASSERT_EQUAL(columns / 8 * 4, panel_map::chips());
        for (int flags = 0; flags < 8; ++flags) {
            panel_map::PanelConfig config;
            config.reverse_ring = flags & 1;
            config.zigzag = flags & 2;
            config.block_transpose = flags & 4;
            std::set<int> seen;
            for (int y = 0; y < life::kHeight; ++y) {
                for (int x = 0; x < columns; ++x) {
                    const panel_map::Address a = panel_map::map_cell(config, x, y);
                    TEST_ASSERT_TRUE(a.chip < panel_map::chips());
                    TEST_ASSERT_TRUE(seen.insert((a.chip * 8 + a.digit) * 8 + a.bit).second);
                }
            }
        }
    }
}

void test_display_modes_stay_inside_the_width() {
    for (int columns : {8, 128}) {
        Width width(columns);
        game::Game game{game::GameConfig{}};
        display_modes::ModeManager modes(game, 3);
        uint32_t now = 0;
        game.start(now);
        for (int m = 0; m < display_modes::kModeCount; ++m) {
            modes.set_mode(static_cast<display_modes::ModeId>(m), now);
            frame::Image image;
            for (int i = 0; i < 50; ++i) {
                modes.tick(now += 20);
                if (i % 10 == 0) {
                    modes.beat(now);
                }
            }
            modes.render(image, now);
            int lit = 0;
            for (int y = 0; y < life::kHeight; ++y) {
                for (int x = 0; x < life::kMaxWidth; ++x) {
                    if (image.get(x, y) != frame::Level::kOff) {
                        TEST_ASSERT_TRUE_MESSAGE(x < columns,
                                                 display_modes::mode_name(modes.mode()));
                        ++lit;
                    }
                }
            }
            TEST_ASSERT_TRUE_MESSAGE(lit > 0 || m == 0, display_modes::mode_name(modes.mode()));
        }
    }
}

void test_frame_size_follows_width() {
    Width width(80);
    frame::Image image;
    image.set(79, 31, frame::Level::kBright);
    uint8_t out[phone_link::kMaxFrameSize];
    phone_link::pack_frame(image, out);
    TEST_ASSERT_EQUAL(80, out[1]);
    TEST_ASSERT_EQUAL(3 + 80 * 32 / 4, static_cast<int>(phone_link::frame_size()));
    TEST_ASSERT_EQUAL_HEX8(0xC0, out[phone_link::frame_size() - 1]);
}

}  // namespace

void run_width_tests() {
    RUN_TEST(test_narrow_ring_skips_wide_shapes_and_presets);
    RUN_TEST(test_wide_ring_fits_everything);
    RUN_TEST(test_panel_map_at_each_width);
    RUN_TEST(test_display_modes_stay_inside_the_width);
    RUN_TEST(test_frame_size_follows_width);
}
