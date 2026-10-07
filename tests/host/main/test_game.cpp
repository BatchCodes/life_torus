// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstring>

#include "game/game.hpp"
#include "patterns/library.hpp"
#include "patterns/presets.hpp"
#include "tests.hpp"
#include "unity.h"

using game::Button;
using game::Game;
using game::GameConfig;
using game::Mode;

namespace {

// The test clock. Each helper moves it forward and ticks the game every 10 ms.
uint32_t g_now = 0;

void wait(Game& game, uint32_t ms) {
    const uint32_t end = g_now + ms;
    while (g_now < end) {
        g_now += 10;
        game.tick(g_now);
    }
}

void tap(Game& game, Button button) {
    g_now += 20;
    game.press(button, g_now);
    g_now += 20;
    game.release(button, g_now);
}

int preset_named(const char* name) {
    const auto all = patterns::presets();
    for (int i = 0; i < static_cast<int>(all.size()); ++i) {
        if (std::strcmp(all[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Starts a game, waits for the start wipe, pauses, and loads the named preset with Select.
void start_paused_at(Game& game, const char* preset) {
    g_now = 1000;
    game.start(g_now);
    wait(game, game.config().transition_ms + 20);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kRun), static_cast<int>(game.mode()));
    tap(game, Button::kStart);  // Any button pauses.
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kPause), static_cast<int>(game.mode()));
    const int target = preset_named(preset);
    TEST_ASSERT_TRUE(target >= 0);
    for (int i = 0; i < 20 && game.preset_index() != target; ++i) {
        tap(game, Button::kSelect);
    }
    TEST_ASSERT_EQUAL(target, game.preset_index());
}

void test_start_wipes_into_run() {
    Game game{GameConfig{}};
    g_now = 0;
    game.start(g_now);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kTransition), static_cast<int>(game.mode()));
    TEST_ASSERT_NOT_EQUAL(static_cast<int>(patterns::PresetKind::kEmpty),
                          static_cast<int>(patterns::presets()[game.preset_index()].kind));
    wait(game, 900);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kRun), static_cast<int>(game.mode()));
}

void test_run_steps_at_step_time() {
    Game game{GameConfig{}};
    start_paused_at(game, "Glider gun");
    tap(game, Button::kStart);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kRun), static_cast<int>(game.mode()));
    wait(game, 1000);
    TEST_ASSERT_INT_WITHIN(1, 10, static_cast<int>(game.simulation().generation()));
}

void test_any_button_pauses_with_no_other_effect() {
    Game game{GameConfig{}};
    start_paused_at(game, "Glider gun");
    tap(game, Button::kStart);
    const int cursor_x = game.cursor_x();
    const int shape = game.shape_index();
    const life::Grid before = game.simulation().current();
    g_now += 20;
    game.press(Button::kRight, g_now);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kPause), static_cast<int>(game.mode()));
    wait(game, 1000);  // Holding the D-pad does not repeat after a pausing press.
    game.release(Button::kRight, g_now);
    TEST_ASSERT_EQUAL(cursor_x, game.cursor_x());
    TEST_ASSERT_EQUAL(shape, game.shape_index());
    TEST_ASSERT_TRUE(game.simulation().current() == before);

    tap(game, Button::kStart);
    tap(game, Button::kR);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kPause), static_cast<int>(game.mode()));
    TEST_ASSERT_EQUAL(shape, game.shape_index());
}

void test_pause_times_out_into_run() {
    Game game{GameConfig{}};
    start_paused_at(game, "Glider gun");
    wait(game, 29000);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kPause), static_cast<int>(game.mode()));
    wait(game, 1100);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kRun), static_cast<int>(game.mode()));
}

void test_cursor_moves_wraps_and_repeats() {
    Game game{GameConfig{}};
    start_paused_at(game, "Empty board");
    const int x = game.cursor_x();
    const int y = game.cursor_y();
    tap(game, Button::kRight);
    TEST_ASSERT_EQUAL(x + 1, game.cursor_x());
    tap(game, Button::kUp);
    TEST_ASSERT_EQUAL(y - 1, game.cursor_y());

    // Hold left: one move, then a repeat every 80 ms after 300 ms.
    g_now += 20;
    game.press(Button::kLeft, g_now);
    TEST_ASSERT_EQUAL(x, game.cursor_x());
    wait(game, 290);
    TEST_ASSERT_EQUAL(x, game.cursor_x());
    wait(game, 20);
    TEST_ASSERT_EQUAL(x - 1, game.cursor_x());
    wait(game, 80 * 40);
    game.release(Button::kLeft, g_now);
    const int after = game.cursor_x();
    TEST_ASSERT_EQUAL(life::wrap_x(x - 41), after);  // Wrapped across the seam.
    wait(game, 500);
    TEST_ASSERT_EQUAL(after, game.cursor_x());
}

void test_single_cell_toggle_and_erase() {
    Game game{GameConfig{}};
    start_paused_at(game, "Empty board");
    const int x = game.cursor_x();
    const int y = game.cursor_y();
    tap(game, Button::kA);
    TEST_ASSERT_TRUE(game.simulation().current().get(x, y));
    tap(game, Button::kA);
    TEST_ASSERT_FALSE(game.simulation().current().get(x, y));
    tap(game, Button::kA);
    tap(game, Button::kB);
    TEST_ASSERT_FALSE(game.simulation().current().get(x, y));
}

void test_shapes_stamp_rotate_and_mirror() {
    Game game{GameConfig{}};
    start_paused_at(game, "Empty board");
    TEST_ASSERT_EQUAL_STRING("Cell", game.shape_name());
    tap(game, Button::kL);
    TEST_ASSERT_EQUAL_STRING("Simkin glider gun", game.shape_name());
    tap(game, Button::kR);
    tap(game, Button::kR);
    TEST_ASSERT_EQUAL_STRING("Block", game.shape_name());
    while (std::strcmp(game.shape_name(), "Glider") != 0) {
        tap(game, Button::kR);
    }
    tap(game, Button::kA);
    TEST_ASSERT_EQUAL(5, game.simulation().current().population());
    const life::Grid one = game.simulation().current();
    tap(game, Button::kB);
    TEST_ASSERT_TRUE(game.simulation().current().empty());

    tap(game, Button::kX);
    tap(game, Button::kA);
    TEST_ASSERT_EQUAL(5, game.simulation().current().population());
    TEST_ASSERT_TRUE(game.simulation().current() != one);
    tap(game, Button::kB);
    tap(game, Button::kY);
    tap(game, Button::kA);
    TEST_ASSERT_EQUAL(5, game.simulation().current().population());
}

void test_select_cycles_presets() {
    Game game{GameConfig{}};
    start_paused_at(game, "Glider gun");
    const int count = static_cast<int>(patterns::presets().size());
    for (int i = 1; i <= count; ++i) {
        tap(game, Button::kSelect);
        TEST_ASSERT_EQUAL(i % count, game.preset_index());
        TEST_ASSERT_EQUAL(static_cast<int>(Mode::kPause), static_cast<int>(game.mode()));
    }
}

void test_empty_board_loads_new_preset() {
    Game game{GameConfig{}};
    start_paused_at(game, "Empty board");
    tap(game, Button::kStart);
    wait(game, 29 * 100 + 50);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kRun), static_cast<int>(game.mode()));
    wait(game, 100);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kTransition), static_cast<int>(game.mode()));
    TEST_ASSERT_NOT_EQUAL(preset_named("Empty board"), game.preset_index());
}

// Draws a shape from the library at the cursor on the empty board, then runs.
void run_with_shape(Game& game, const char* shape) {
    start_paused_at(game, "Empty board");
    while (std::strcmp(game.shape_name(), shape) != 0) {
        tap(game, Button::kR);
    }
    tap(game, Button::kA);
    tap(game, Button::kStart);
}

void test_still_board_loads_new_preset() {
    Game game{GameConfig{}};
    run_with_shape(game, "Block");
    wait(game, 29 * 100 + 50);
    TEST_ASSERT_EQUAL(preset_named("Empty board"), game.preset_index());
    wait(game, 100);
    TEST_ASSERT_NOT_EQUAL(preset_named("Empty board"), game.preset_index());
}

void test_period_two_board_loads_new_preset() {
    Game game{GameConfig{}};
    run_with_shape(game, "Blinker");
    wait(game, 29 * 100 + 50);
    TEST_ASSERT_EQUAL(preset_named("Empty board"), game.preset_index());
    wait(game, 200);
    TEST_ASSERT_NOT_EQUAL(preset_named("Empty board"), game.preset_index());
}

void test_moving_board_is_not_settled() {
    Game game{GameConfig{}};
    run_with_shape(game, "Glider");
    wait(game, 200 * 100);
    TEST_ASSERT_EQUAL(preset_named("Empty board"), game.preset_index());
}

void test_repeat_loads_new_preset() {
    Game game{GameConfig{}};
    // The garden repeats with period 30 from the start. The rule needs 300 generations in a
    // repeat. The first repeat is at generation 30.
    start_paused_at(game, "Oscillator garden");
    tap(game, Button::kStart);
    wait(game, 320 * 100);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kRun), static_cast<int>(game.mode()));
    wait(game, 20 * 100);
    TEST_ASSERT_NOT_EQUAL(preset_named("Oscillator garden"), game.preset_index());
}

void test_repeat_rule_can_be_off() {
    GameConfig config;
    config.repeat_limit = 0;
    Game game{config};
    start_paused_at(game, "Oscillator garden");
    tap(game, Button::kStart);
    wait(game, 400 * 100);
    TEST_ASSERT_EQUAL(preset_named("Oscillator garden"), game.preset_index());
}

void test_no_input_limit_loads_new_preset() {
    GameConfig config;
    config.no_input_limit = 50;
    Game game{config};
    start_paused_at(game, "Glider gun");
    tap(game, Button::kStart);
    wait(game, 49 * 100);
    TEST_ASSERT_EQUAL(preset_named("Glider gun"), game.preset_index());
    wait(game, 200);
    TEST_ASSERT_NOT_EQUAL(preset_named("Glider gun"), game.preset_index());
}

void enter_ko_code(Game& game, int first) {
    constexpr Button kSequence[] = {Button::kUp,   Button::kUp,    Button::kDown, Button::kDown,
                                    Button::kLeft, Button::kRight, Button::kLeft, Button::kRight,
                                    Button::kB,    Button::kA};
    for (int i = first; i < 10; ++i) {
        tap(game, kSequence[i]);
    }
}

void test_ko_code_in_pause_restores_board() {
    Game game{GameConfig{}};
    start_paused_at(game, "Empty board");
    tap(game, Button::kA);  // One live cell under the cursor.
    const life::Grid board = game.simulation().current();
    const int x = game.cursor_x();
    const int y = game.cursor_y();
    enter_ko_code(game, 0);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kKoCode), static_cast<int>(game.mode()));
    TEST_ASSERT_TRUE(game.simulation().current() == board);  // B and A are undone.
    TEST_ASSERT_EQUAL(x, game.cursor_x());
    TEST_ASSERT_EQUAL(y, game.cursor_y());

    frame::Image image;
    game.render(image, g_now);
    tap(game, Button::kStart);  // Ignored during the effect.
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kKoCode), static_cast<int>(game.mode()));
    wait(game, 10000);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kPause), static_cast<int>(game.mode()));
    TEST_ASSERT_TRUE(game.simulation().current() == board);
}

void test_ko_code_from_run_returns_to_run() {
    Game game{GameConfig{}};
    start_paused_at(game, "Glider gun");
    tap(game, Button::kStart);
    enter_ko_code(game, 0);  // The first press pauses.
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kKoCode), static_cast<int>(game.mode()));
    wait(game, 10010);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kRun), static_cast<int>(game.mode()));
}

void test_ko_code_wrong_button_and_gap() {
    Game game{GameConfig{}};
    start_paused_at(game, "Empty board");
    tap(game, Button::kUp);
    tap(game, Button::kUp);
    tap(game, Button::kX);  // Wrong button.
    enter_ko_code(game, 2);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kPause), static_cast<int>(game.mode()));

    tap(game, Button::kUp);
    tap(game, Button::kUp);
    tap(game, Button::kDown);
    wait(game, 2100);  // Gap too long.
    enter_ko_code(game, 3);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kPause), static_cast<int>(game.mode()));
}

void test_ko_code_after_extra_first_button() {
    Game game{GameConfig{}};
    start_paused_at(game, "Empty board");
    tap(game, Button::kUp);  // Three times up, then the rest of the code.
    enter_ko_code(game, 0);
    TEST_ASSERT_EQUAL(static_cast<int>(Mode::kKoCode), static_cast<int>(game.mode()));
}

}  // namespace

void run_game_tests() {
    RUN_TEST(test_start_wipes_into_run);
    RUN_TEST(test_run_steps_at_step_time);
    RUN_TEST(test_any_button_pauses_with_no_other_effect);
    RUN_TEST(test_pause_times_out_into_run);
    RUN_TEST(test_cursor_moves_wraps_and_repeats);
    RUN_TEST(test_single_cell_toggle_and_erase);
    RUN_TEST(test_shapes_stamp_rotate_and_mirror);
    RUN_TEST(test_select_cycles_presets);
    RUN_TEST(test_empty_board_loads_new_preset);
    RUN_TEST(test_still_board_loads_new_preset);
    RUN_TEST(test_period_two_board_loads_new_preset);
    RUN_TEST(test_moving_board_is_not_settled);
    RUN_TEST(test_repeat_loads_new_preset);
    RUN_TEST(test_repeat_rule_can_be_off);
    RUN_TEST(test_no_input_limit_loads_new_preset);
    RUN_TEST(test_ko_code_in_pause_restores_board);
    RUN_TEST(test_ko_code_from_run_returns_to_run);
    RUN_TEST(test_ko_code_wrong_button_and_gap);
    RUN_TEST(test_ko_code_after_extra_first_button);
}
