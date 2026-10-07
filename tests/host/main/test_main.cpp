// SPDX-License-Identifier: GPL-3.0-or-later
#include "tests.hpp"
#include "unity.h"

extern "C" void setUp() {}
extern "C" void tearDown() {}

int main() {
    UNITY_BEGIN();
    run_life_tests();
    run_patterns_tests();
    return UNITY_END();
}
