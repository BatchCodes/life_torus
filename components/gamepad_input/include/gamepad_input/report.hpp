// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>

#include "game/game.hpp"

namespace gamepad_input {

constexpr int kButtonCount = 12;
using ButtonMask = uint16_t;  // Bit n is game::Button n.

constexpr ButtonMask bit(game::Button button) {
    return static_cast<ButtonMask>(1u << static_cast<int>(button));
}

struct ButtonBit {
    game::Button button;
    uint8_t byte;
    uint8_t mask;
};

// Where a USB HID gamepad puts its axes and buttons in its input report.
struct ReportLayout {
    const char* name;
    uint8_t min_length;
    uint8_t x_axis_byte;  // 0x00: left, 0x7F: centre, 0xFF: right.
    uint8_t y_axis_byte;  // 0x00: up, 0x7F: centre, 0xFF: down.
    ButtonBit buttons[8];
};

// Generic USB SNES controllers (for example DragonRise 0079:0011, sold under many names such
// as Rii). Step 1.10 of the bring-up confirms the layout with the Rii controller.
extern const ReportLayout kGenericSnes;

// Converts one input report to a button mask. Returns false if the report is too short.
bool parse_report(const ReportLayout& layout, const uint8_t* data, size_t length,
                  ButtonMask& buttons);

// Calls on_change(button, pressed) for each button that differs between `before` and `after`.
template <typename Fn>
void for_each_change(ButtonMask before, ButtonMask after, Fn&& on_change) {
    const ButtonMask changed = before ^ after;
    for (int i = 0; i < kButtonCount; ++i) {
        const auto button = static_cast<game::Button>(i);
        if (changed & bit(button)) {
            on_change(button, (after & bit(button)) != 0);
        }
    }
}

}  // namespace gamepad_input
