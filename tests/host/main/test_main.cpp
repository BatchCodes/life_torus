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
    return UNITY_END();
}
