// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>

#include "frame/image.hpp"

namespace panel_map {

constexpr int kBoards = 8;
constexpr int kChipsPerBoard = 4;
constexpr int kChips = kBoards * kChipsPerBoard;
constexpr int kDigits = 8;
constexpr int kBoardColumns = 8;

// How the boards stand in the ring and how each 8 × 8 block connects to its MAX7219.
//
// Board 0 is the first board in the chain (the board that the ESP32 connects to). Each board
// stands upright: 8 columns wide and 32 rows high. Chip 0 of a board is the chip nearest to its
// input connector. An upright board has its input connector at the bottom.
struct PanelConfig {
    // Board 0 is at columns 0 to 7 and the chain goes left to right around the ring. With
    // reverse_ring, the chain goes right to left (board 0 at columns 56 to 63).
    bool reverse_ring = false;
    // All boards stand upside down (input connector at the top).
    bool flip_boards = false;
    // Every second board (1, 3, 5, 7) is turned by 180 degrees compared to board 0. This
    // keeps the board-to-board wires short.
    bool zigzag = false;
    // Inside an upright block, digit register d is column d and segment bit b is row b, from
    // the top-left corner. These flags change that, to match the real boards.
    bool block_transpose = false;
    bool block_flip_x = false;
    bool block_flip_y = false;
};

struct Address {
    uint8_t chip;   // 0: first chip in the chain.
    uint8_t digit;  // 0 to 7. The MAX7219 register is digit + 1.
    uint8_t bit;    // 0 to 7. Bit 7 is segment DP, bit 0 is segment G.
};

Address map_cell(const PanelConfig& config, int x, int y);

// The register data of one full display: data[digit][chip].
using Registers = std::array<std::array<uint8_t, kChips>, kDigits>;

// True if a cell with this level is on in sub-frame `subframe` of `subframes`. With levels,
// bright cells are on in all sub-frames, normal cells in two thirds and dim cells in one third.
// Without levels, every cell that is not off is on in all sub-frames.
bool cell_on(frame::Level level, int subframe, int subframes, bool levels);

// Converts an image to the registers of one sub-frame.
void encode(const PanelConfig& config, const frame::Image& image, int subframe, int subframes,
            bool levels, Registers& out);

}  // namespace panel_map
