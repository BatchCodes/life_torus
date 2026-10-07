// SPDX-License-Identifier: GPL-3.0-or-later
#include <vector>

#include "gamepad_input/report.hpp"
#include "tests.hpp"
#include "unity.h"

using game::Button;
using gamepad_input::bit;
using gamepad_input::ButtonMask;
using gamepad_input::kGenericSnes;

namespace {

// Reports of a generic USB SNES controller.
constexpr uint8_t kIdle[] = {0x01, 0x7F, 0x7F, 0x7F, 0x7F, 0x0F, 0x00, 0x00};
constexpr uint8_t kLeftUp[] = {0x01, 0x7F, 0x7F, 0x00, 0x00, 0x0F, 0x00, 0x00};
constexpr uint8_t kRightDownA[] = {0x01, 0x7F, 0x7F, 0xFF, 0xFF, 0x2F, 0x00, 0x00};
constexpr uint8_t kXBY[] = {0x01, 0x7F, 0x7F, 0x7F, 0x7F, 0xDF, 0x00, 0x00};
constexpr uint8_t kShoulderSelectStart[] = {0x01, 0x7F, 0x7F, 0x7F, 0x7F, 0x0F, 0x33, 0x00};

ButtonMask parse(const uint8_t* data, size_t length) {
    ButtonMask mask = 0xFFFF;
    TEST_ASSERT_TRUE(gamepad_input::parse_report(kGenericSnes, data, length, mask));
    return mask;
}

void test_idle_report() {
    TEST_ASSERT_EQUAL_HEX16(0, parse(kIdle, sizeof(kIdle)));
}

void test_dpad() {
    TEST_ASSERT_EQUAL_HEX16(bit(Button::kLeft) | bit(Button::kUp), parse(kLeftUp, sizeof(kLeftUp)));
    TEST_ASSERT_EQUAL_HEX16(bit(Button::kRight) | bit(Button::kDown) | bit(Button::kA),
                            parse(kRightDownA, sizeof(kRightDownA)));
}

void test_face_and_shoulder_buttons() {
    TEST_ASSERT_EQUAL_HEX16(bit(Button::kX) | bit(Button::kB) | bit(Button::kY),
                            parse(kXBY, sizeof(kXBY)));
    TEST_ASSERT_EQUAL_HEX16(
        bit(Button::kL) | bit(Button::kR) | bit(Button::kSelect) | bit(Button::kStart),
        parse(kShoulderSelectStart, sizeof(kShoulderSelectStart)));
}

void test_short_report_is_rejected() {
    ButtonMask mask = 0;
    TEST_ASSERT_FALSE(gamepad_input::parse_report(kGenericSnes, kIdle, 4, mask));
}

void test_changes() {
    struct Change {
        Button button;
        bool pressed;
    };
    std::vector<Change> changes;
    const auto record = [&changes](Button b, bool pressed) { changes.push_back({b, pressed}); };
    gamepad_input::for_each_change(0, bit(Button::kA) | bit(Button::kUp), record);
    TEST_ASSERT_EQUAL(2, static_cast<int>(changes.size()));
    TEST_ASSERT_EQUAL(static_cast<int>(Button::kUp), static_cast<int>(changes[0].button));
    TEST_ASSERT_TRUE(changes[0].pressed);
    changes.clear();
    gamepad_input::for_each_change(bit(Button::kA) | bit(Button::kUp), bit(Button::kA), record);
    TEST_ASSERT_EQUAL(1, static_cast<int>(changes.size()));
    TEST_ASSERT_EQUAL(static_cast<int>(Button::kUp), static_cast<int>(changes[0].button));
    TEST_ASSERT_FALSE(changes[0].pressed);
}

}  // namespace

void run_gamepad_report_tests() {
    RUN_TEST(test_idle_report);
    RUN_TEST(test_dpad);
    RUN_TEST(test_face_and_shoulder_buttons);
    RUN_TEST(test_short_report_is_rejected);
    RUN_TEST(test_changes);
}
