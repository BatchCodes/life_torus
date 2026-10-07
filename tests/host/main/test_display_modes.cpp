// SPDX-License-Identifier: GPL-3.0-or-later
#include "display_modes/font.hpp"
#include "display_modes/mode_manager.hpp"
#include "tests.hpp"
#include "unity.h"

using display_modes::ModeId;
using display_modes::ModeManager;
using frame::Image;
using frame::Level;

namespace {

int count_lit(const Image& image) {
    int n = 0;
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::width(); ++x) {
            if (image.get(x, y) != Level::kOff) {
                ++n;
            }
        }
    }
    return n;
}

bool uses_level(const Image& image, Level level) {
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::width(); ++x) {
            if (image.get(x, y) == level) {
                return true;
            }
        }
    }
    return false;
}

struct Rig {
    game::Game game{game::GameConfig{}};
    ModeManager modes{game, 7};
    uint32_t now = 1000;
    Image image;

    Rig() { game.start(now); }

    void run(uint32_t ms) {
        const uint32_t end = now + ms;
        while (now < end) {
            now += 20;
            modes.tick(now);
        }
        modes.render(image, now);
    }
};

void test_font() {
    // 'A' has a full left column and a cross bar.
    TEST_ASSERT_EQUAL_HEX8(0x7E, display_modes::font_column('A', 0));
    TEST_ASSERT_EQUAL_HEX8(display_modes::font_column('?', 0),
                           display_modes::font_column('\x01', 0));
    TEST_ASSERT_TRUE(display_modes::large_char_cell('I', 2 * 3, 0));
    TEST_ASSERT_FALSE(display_modes::large_char_cell('I', 0, 0));
    TEST_ASSERT_FALSE(display_modes::large_char_cell(' ', 6, 6));
}

void test_starts_in_game_of_life() {
    Rig rig;
    TEST_ASSERT_EQUAL(static_cast<int>(ModeId::kGameOfLife), static_cast<int>(rig.modes.mode()));
    rig.run(2000);
    TEST_ASSERT_TRUE(rig.game.simulation().generation() > 0);
}

void test_each_mode_draws_and_moves() {
    for (int m = 1; m < display_modes::kModeCount; ++m) {
        Rig rig;
        rig.modes.set_mode(static_cast<ModeId>(m), rig.now);
        rig.run(500);
        const Image first = rig.image;
        TEST_ASSERT_TRUE_MESSAGE(count_lit(first) > 0, display_modes::mode_name(rig.modes.mode()));
        rig.run(500);
        TEST_ASSERT_FALSE_MESSAGE(rig.image == first, display_modes::mode_name(rig.modes.mode()));
    }
}

void test_effects_use_levels() {
    const ModeId modes[] = {ModeId::kRain, ModeId::kBarberPole, ModeId::kRipples, ModeId::kSparkle};
    for (ModeId m : modes) {
        Rig rig;
        rig.modes.set_mode(m, rig.now);
        rig.run(1500);
        TEST_ASSERT_TRUE_MESSAGE(uses_level(rig.image, Level::kBright),
                                 display_modes::mode_name(m));
        TEST_ASSERT_TRUE_MESSAGE(uses_level(rig.image, Level::kDim), display_modes::mode_name(m));
    }
}

void test_text_scrolls_at_speed() {
    Rig rig;
    rig.modes.set_text("HI");
    rig.modes.set_speed(5);
    rig.modes.set_mode(ModeId::kText, rig.now);
    rig.run(20);
    const Image a = rig.image;
    // Speed 5 is 20 columns per second: 1 column in 50 ms.
    rig.run(1000);
    const Image b = rig.image;
    int matches = 0;
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x + 20 < life::width(); ++x) {
            if (a.get(x + 20, y) == b.get(x, y)) {
                ++matches;
            }
        }
    }
    TEST_ASSERT_EQUAL(life::kHeight * (life::width() - 20), matches);
    TEST_ASSERT_EQUAL_STRING("HI", rig.modes.text());
}

void test_controller_does_nothing_in_other_modes() {
    Rig rig;
    rig.modes.set_mode(ModeId::kRain, rig.now);
    const auto mode_before = rig.game.mode();
    rig.modes.press(game::Button::kStart, rig.now);
    rig.modes.press(game::Button::kA, rig.now + 50);
    TEST_ASSERT_EQUAL(static_cast<int>(ModeId::kRain), static_cast<int>(rig.modes.mode()));
    TEST_ASSERT_EQUAL(static_cast<int>(mode_before), static_cast<int>(rig.game.mode()));
}

void test_ko_code_works_in_other_modes() {
    Rig rig;
    rig.modes.set_mode(ModeId::kSparkle, rig.now);
    constexpr game::Button kSequence[] = {
        game::Button::kUp,   game::Button::kUp,    game::Button::kDown, game::Button::kDown,
        game::Button::kLeft, game::Button::kRight, game::Button::kLeft, game::Button::kRight,
        game::Button::kB,    game::Button::kA};
    for (game::Button b : kSequence) {
        rig.now += 100;
        rig.modes.press(b, rig.now);
    }
    TEST_ASSERT_TRUE(rig.modes.ko_effect_on());
    rig.run(10100);
    TEST_ASSERT_FALSE(rig.modes.ko_effect_on());
    TEST_ASSERT_EQUAL(static_cast<int>(ModeId::kSparkle), static_cast<int>(rig.modes.mode()));
}

void test_back_to_game_of_life_keeps_board() {
    Rig rig;
    rig.run(1000);
    const uint32_t generation = rig.game.simulation().generation();
    rig.modes.set_mode(ModeId::kBarberPole, rig.now);
    rig.run(3000);
    TEST_ASSERT_EQUAL_UINT32(generation, rig.game.simulation().generation());
    rig.modes.set_mode(ModeId::kGameOfLife, rig.now);
    rig.run(1000);
    TEST_ASSERT_TRUE(rig.game.simulation().generation() > generation);
}

void test_speed_is_limited() {
    Rig rig;
    rig.modes.set_speed(99);
    TEST_ASSERT_EQUAL(display_modes::kMaxSpeed, rig.modes.speed());
    rig.modes.set_speed(-3);
    TEST_ASSERT_EQUAL(display_modes::kMinSpeed, rig.modes.speed());
}

// Runs a mode for 3 s with a beat every 500 ms (120 BPM), and returns the number of lit cells
// right after the last beat.
int lit_after_beats(ModeId mode, bool beats) {
    Rig rig;
    rig.modes.set_mode(mode, rig.now);
    for (int i = 0; i < 6; ++i) {
        rig.run(500);
        if (beats) {
            rig.modes.beat(rig.now);
        }
    }
    rig.run(20);
    return count_lit(rig.image);
}

void test_beats_add_activity() {
    TEST_ASSERT_TRUE(lit_after_beats(ModeId::kSparkle, true) >
                     lit_after_beats(ModeId::kSparkle, false));
    TEST_ASSERT_TRUE(lit_after_beats(ModeId::kRipples, true) > 0);
}

void test_beats_active_only_while_beats_come() {
    Rig rig;
    rig.modes.set_mode(ModeId::kBarberPole, rig.now);
    TEST_ASSERT_FALSE(rig.modes.beats_active(rig.now));
    rig.modes.beat(rig.now);
    rig.run(500);
    rig.modes.beat(rig.now);
    TEST_ASSERT_TRUE(rig.modes.beats_active(rig.now));
    rig.run(1100);
    TEST_ASSERT_FALSE(rig.modes.beats_active(rig.now));
}

void test_flash_on_beat() {
    Rig rig;
    rig.modes.set_text("I");
    rig.modes.set_mode(ModeId::kText, rig.now);
    rig.modes.beat(rig.now);
    rig.run(500);
    rig.modes.beat(rig.now);
    rig.run(20);
    TEST_ASSERT_FALSE(uses_level(rig.image, Level::kNormal));  // Flash: the letters are bright.
    rig.run(200);
    TEST_ASSERT_FALSE(uses_level(rig.image, Level::kBright));  // After the flash: normal.
}

void test_barber_pole_moves_with_the_beat() {
    Rig rig;
    rig.modes.set_mode(ModeId::kBarberPole, rig.now);
    rig.modes.beat(rig.now);
    rig.run(400);
    rig.modes.beat(rig.now);  // Beat period 400 ms.
    rig.run(200);
    const Image a = rig.image;
    rig.run(400);  // One beat: one full stripe of 8 columns, so the same picture.
    // Compare outside the flash time.
    rig.run(0);
    int same = 0;
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::width(); ++x) {
            same += a.get(x, y) == rig.image.get(x, y) ? 1 : 0;
        }
    }
    TEST_ASSERT_TRUE(same > life::width() * life::kHeight * 3 / 4);
}

void test_game_of_life_ignores_beats() {
    Rig rig;
    rig.run(1000);
    const uint32_t generation = rig.game.simulation().generation();
    rig.modes.beat(rig.now);
    TEST_ASSERT_EQUAL_UINT32(generation, rig.game.simulation().generation());
}

void test_next_mode_cycles() {
    Rig rig;
    for (int i = 1; i <= display_modes::kModeCount; ++i) {
        rig.modes.next_mode(rig.now);
        TEST_ASSERT_EQUAL(i % display_modes::kModeCount, static_cast<int>(rig.modes.mode()));
    }
}

void test_visualiser_shows_bands() {
    Rig rig;
    rig.modes.set_mode(ModeId::kVisualiser, rig.now);
    float levels[32] = {};
    levels[0] = 1.0f;   // Full bar in columns 0 and 1.
    levels[10] = 0.5f;  // Half bar in columns 20 and 21.
    rig.modes.set_spectrum(levels, rig.now);
    rig.run(20);
    TEST_ASSERT_EQUAL(static_cast<int>(Level::kBright), static_cast<int>(rig.image.get(0, 0)));
    TEST_ASSERT_EQUAL(static_cast<int>(Level::kNormal), static_cast<int>(rig.image.get(1, 31)));
    TEST_ASSERT_EQUAL(static_cast<int>(Level::kBright), static_cast<int>(rig.image.get(20, 16)));
    TEST_ASSERT_EQUAL(static_cast<int>(Level::kOff), static_cast<int>(rig.image.get(20, 15)));
    TEST_ASSERT_EQUAL(static_cast<int>(Level::kOff), static_cast<int>(rig.image.get(4, 31)));

    // The bar drops at once. The peak marker stays a moment and falls slowly.
    levels[10] = 0.0f;
    rig.modes.set_spectrum(levels, rig.now);
    rig.run(100);
    TEST_ASSERT_EQUAL(static_cast<int>(Level::kOff), static_cast<int>(rig.image.get(20, 31)));
    bool marker = false;
    for (int y = 14; y < 20; ++y) {
        marker = marker || rig.image.get(20, y) == Level::kDim;
    }
    TEST_ASSERT_TRUE(marker);
}

void test_visualiser_idle_wave() {
    Rig rig;
    rig.modes.set_mode(ModeId::kVisualiser, rig.now);
    rig.run(500);
    const Image a = rig.image;
    TEST_ASSERT_TRUE(count_lit(a) > 200);  // Not dark with no audio.
    rig.run(1000);
    TEST_ASSERT_FALSE(rig.image == a);  // The wave moves.
}

}  // namespace

void run_display_modes_tests() {
    RUN_TEST(test_visualiser_shows_bands);
    RUN_TEST(test_visualiser_idle_wave);
    RUN_TEST(test_next_mode_cycles);
    RUN_TEST(test_beats_add_activity);
    RUN_TEST(test_beats_active_only_while_beats_come);
    RUN_TEST(test_flash_on_beat);
    RUN_TEST(test_barber_pole_moves_with_the_beat);
    RUN_TEST(test_game_of_life_ignores_beats);
    RUN_TEST(test_font);
    RUN_TEST(test_starts_in_game_of_life);
    RUN_TEST(test_each_mode_draws_and_moves);
    RUN_TEST(test_effects_use_levels);
    RUN_TEST(test_text_scrolls_at_speed);
    RUN_TEST(test_controller_does_nothing_in_other_modes);
    RUN_TEST(test_ko_code_works_in_other_modes);
    RUN_TEST(test_back_to_game_of_life_keeps_board);
    RUN_TEST(test_speed_is_limited);
}
