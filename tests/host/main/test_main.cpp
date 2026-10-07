// SPDX-License-Identifier: GPL-3.0-or-later
#include "tests.hpp"
#include "unity.h"

extern "C" void setUp() {}
extern "C" void tearDown() {}

int main() {
    UNITY_BEGIN();
    run_life_tests();
    run_patterns_tests();
    run_frame_tests();
    run_game_tests();
    run_panel_map_tests();
    run_gamepad_report_tests();
    run_display_modes_tests();
    run_settings_tests();
    run_phone_protocol_tests();
    run_beat_tests();
    run_debouncer_tests();
    return UNITY_END();
}
