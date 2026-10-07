// SPDX-License-Identifier: GPL-3.0-or-later
#include "gamepad_input/report.hpp"

namespace gamepad_input {

namespace {

constexpr uint8_t kAxisLow = 0x40;
constexpr uint8_t kAxisHigh = 0xC0;

}  // namespace

// Idle report: 01 7F 7F 7F 7F 0F 00 00. Byte 3 is X, byte 4 is Y. Byte 5 has X, A, B and Y in
// its high nibble. Byte 6 has L, R, Select and Start.
const ReportLayout kGenericSnes = {
    "generic USB SNES",
    7,
    3,
    4,
    {
        {game::Button::kX, 5, 0x10},
        {game::Button::kA, 5, 0x20},
        {game::Button::kB, 5, 0x40},
        {game::Button::kY, 5, 0x80},
        {game::Button::kL, 6, 0x01},
        {game::Button::kR, 6, 0x02},
        {game::Button::kSelect, 6, 0x10},
        {game::Button::kStart, 6, 0x20},
    },
};

bool parse_report(const ReportLayout& layout, const uint8_t* data, size_t length,
                  ButtonMask& buttons) {
    if (length < layout.min_length) {
        return false;
    }
    ButtonMask mask = 0;
    const uint8_t x = data[layout.x_axis_byte];
    const uint8_t y = data[layout.y_axis_byte];
    if (x < kAxisLow) {
        mask |= bit(game::Button::kLeft);
    } else if (x > kAxisHigh) {
        mask |= bit(game::Button::kRight);
    }
    if (y < kAxisLow) {
        mask |= bit(game::Button::kUp);
    } else if (y > kAxisHigh) {
        mask |= bit(game::Button::kDown);
    }
    for (const ButtonBit& b : layout.buttons) {
        if (data[b.byte] & b.mask) {
            mask |= bit(b.button);
        }
    }
    buttons = mask;
    return true;
}

}  // namespace gamepad_input
