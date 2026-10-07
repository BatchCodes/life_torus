// SPDX-License-Identifier: GPL-3.0-or-later
#include "mode_button/debouncer.hpp"
#include "tests.hpp"
#include "unity.h"

using mode_button::Debouncer;

namespace {

void test_press_needs_stable_time() {
    Debouncer d(30);
    TEST_ASSERT_FALSE(d.update(true, 0));
    TEST_ASSERT_FALSE(d.update(true, 29));
    TEST_ASSERT_TRUE(d.update(true, 30));
    TEST_ASSERT_FALSE(d.update(true, 40));  // Only one press per push.
    TEST_ASSERT_TRUE(d.pressed());
}

void test_bounce_gives_one_press() {
    Debouncer d(30);
    int presses = 0;
    // The contact bounces for 10 ms, then stays closed, then opens and bounces again.
    const bool levels[] = {true,  false, true,  false, true,  true,  true,  true,
                           true,  true,  true,  true,  false, true,  false, false,
                           false, false, false, false, false, false, false, false};
    uint32_t t = 0;
    for (int repeat = 0; repeat < 4; ++repeat) {
        for (bool level : levels) {
            presses += d.update(level, t) ? 1 : 0;
            t += 5;
        }
    }
    // Each 120 ms cycle stays closed for 40 ms and open for 45 ms: one press each.
    TEST_ASSERT_EQUAL(4, presses);
}

void test_short_glitch_is_ignored() {
    Debouncer d(30);
    d.update(true, 0);
    d.update(false, 10);
    TEST_ASSERT_FALSE(d.update(false, 50));
    TEST_ASSERT_FALSE(d.pressed());
}

void test_release_then_press_again() {
    Debouncer d(30);
    d.update(true, 0);
    TEST_ASSERT_TRUE(d.update(true, 30));
    d.update(false, 100);
    TEST_ASSERT_FALSE(d.update(false, 130));
    TEST_ASSERT_FALSE(d.pressed());
    d.update(true, 200);
    TEST_ASSERT_TRUE(d.update(true, 230));
}

}  // namespace

void run_debouncer_tests() {
    RUN_TEST(test_press_needs_stable_time);
    RUN_TEST(test_bounce_gives_one_press);
    RUN_TEST(test_short_glitch_is_ignored);
    RUN_TEST(test_release_then_press_again);
}
