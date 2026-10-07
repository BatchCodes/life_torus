// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstring>

#include "settings/settings.hpp"
#include "tests.hpp"
#include "unity.h"

using settings::Settings;

namespace {

void test_round_trip() {
    Settings a;
    a.step_ms = 250;
    a.intensity = 7;
    a.brightness_levels = false;
    a.cylinder = true;
    a.panel.zigzag = true;
    a.panel.block_flip_y = true;
    std::strcpy(a.password, "pa ss&word=1");
    char text[settings::kMaxText];
    TEST_ASSERT_TRUE(settings::to_text(a, text, sizeof(text), true) > 0);
    TEST_ASSERT_NOT_NULL(std::strstr(text, "password=pa%20ss%26word%3D1"));

    Settings b;
    TEST_ASSERT_TRUE(settings::from_text(text, b));
    TEST_ASSERT_EQUAL_UINT32(250, b.step_ms);
    TEST_ASSERT_EQUAL_UINT32(7, b.intensity);
    TEST_ASSERT_FALSE(b.brightness_levels);
    TEST_ASSERT_TRUE(b.cylinder);
    TEST_ASSERT_TRUE(b.panel.zigzag);
    TEST_ASSERT_TRUE(b.panel.block_flip_y);
    TEST_ASSERT_FALSE(b.panel.reverse_ring);
    TEST_ASSERT_EQUAL_STRING("pa ss&word=1", b.password);
}

void test_password_left_out() {
    Settings a;
    char text[settings::kMaxText];
    settings::to_text(a, text, sizeof(text), false);
    TEST_ASSERT_NULL(std::strstr(text, "password"));
}

void test_partial_and_unknown_keys() {
    Settings s;
    TEST_ASSERT_TRUE(settings::from_text("intensity=9&future_option=3", s));
    TEST_ASSERT_EQUAL_UINT32(9, s.intensity);
    TEST_ASSERT_EQUAL_UINT32(100, s.step_ms);
}

void test_values_are_clamped() {
    Settings s;
    TEST_ASSERT_TRUE(settings::from_text("intensity=99&step_ms=1&random_percent=0", s));
    TEST_ASSERT_EQUAL_UINT32(15, s.intensity);
    TEST_ASSERT_EQUAL_UINT32(20, s.step_ms);
    TEST_ASSERT_EQUAL_UINT32(1, s.random_percent);
}

void test_bad_text_changes_nothing() {
    Settings s;
    TEST_ASSERT_FALSE(settings::from_text("intensity=9&step_ms=abc", s));
    TEST_ASSERT_EQUAL_UINT32(4, s.intensity);
    TEST_ASSERT_FALSE(settings::from_text("intensity", s));
    TEST_ASSERT_FALSE(settings::from_text("zigzag=2", s));
    TEST_ASSERT_FALSE(settings::from_text("password=", s));
    TEST_ASSERT_FALSE(settings::from_text("password=%4", s));
    TEST_ASSERT_EQUAL_STRING("life", s.password);
}

void test_small_buffer() {
    Settings s;
    char text[16];
    TEST_ASSERT_EQUAL(0, static_cast<int>(settings::to_text(s, text, sizeof(text), true)));
}

void test_apply() {
    Settings s;
    s.step_ms = 300;
    s.cylinder = true;
    s.brightness_levels = false;
    game::GameConfig config;
    settings::apply(s, config);
    TEST_ASSERT_EQUAL_UINT32(300, config.step_ms);
    TEST_ASSERT_EQUAL(static_cast<int>(life::EdgeMode::kCylinder),
                      static_cast<int>(config.default_edge_mode));
    TEST_ASSERT_FALSE(config.brightness_levels);
}

}  // namespace

void run_settings_tests() {
    RUN_TEST(test_round_trip);
    RUN_TEST(test_password_left_out);
    RUN_TEST(test_partial_and_unknown_keys);
    RUN_TEST(test_values_are_clamped);
    RUN_TEST(test_bad_text_changes_nothing);
    RUN_TEST(test_small_buffer);
    RUN_TEST(test_apply);
}
