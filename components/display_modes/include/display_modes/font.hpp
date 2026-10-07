// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

#include "frame/image.hpp"

namespace display_modes {

// A 5 × 7 font for printable ASCII (0x20 to 0x7E), shown three times as large: 15 × 21 cells,
// with 3 empty columns after each character.
constexpr int kFontScale = 3;
constexpr int kCharWidth = 5 * kFontScale;
constexpr int kCharAdvance = (5 + 1) * kFontScale;
constexpr int kCharHeight = 7 * kFontScale;

// Column `column` (0 to 4) of a character. Bit 0 is the top row. Other characters show as '?'.
uint8_t font_column(char c, int column);

// True if the large character has a lit cell at (x, y), 0 <= x < kCharAdvance, 0 <= y <
// kCharHeight.
bool large_char_cell(char c, int x, int y);

}  // namespace display_modes
