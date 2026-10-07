// SPDX-License-Identifier: GPL-3.0-or-later
#include "panel_map/panel_map.hpp"

namespace panel_map {

Address map_cell(const PanelConfig& config, int x, int y) {
    int board = x / kBoardColumns;
    if (config.reverse_ring) {
        board = boards() - 1 - board;
    }
    // Position inside an upright board: u from the left, v from the top.
    // The columns inside a board stay left to right. Only the board order reverses.
    int u = x % kBoardColumns;
    int v = y;
    bool upside_down = config.flip_boards;
    if (config.zigzag && (board % 2) == 1) {
        upside_down = !upside_down;
    }
    if (upside_down) {
        u = kBoardColumns - 1 - u;
        v = life::kHeight - 1 - v;
    }
    // Upright: chip 0 is at the bottom, next to the input connector.
    const int chip_in_board = (life::kHeight - 1 - v) / 8;
    int bx = u;      // Column inside the block, from the left.
    int by = v % 8;  // Row inside the block, from the top.
    if (config.block_flip_x) {
        bx = 7 - bx;
    }
    if (config.block_flip_y) {
        by = 7 - by;
    }
    int digit = bx;
    int bit = by;
    if (config.block_transpose) {
        digit = by;
        bit = bx;
    }
    return Address{static_cast<uint8_t>(board * kChipsPerBoard + chip_in_board),
                   static_cast<uint8_t>(digit), static_cast<uint8_t>(bit)};
}

bool cell_on(frame::Level level, int subframe, int subframes, bool levels) {
    if (level == frame::Level::kOff) {
        return false;
    }
    if (!levels || subframes <= 1) {
        return true;
    }
    // Number of sub-frames that this level is on, rounded up: 1/3, 2/3 or 3/3.
    const int duty = (subframes * frame::brightness(level) + 2) / 3;
    return subframe < duty;
}

void encode(const PanelConfig& config, const frame::Image& image, int subframe, int subframes,
            bool levels, Registers& out) {
    for (auto& digit : out) {
        digit.fill(0);
    }
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::width(); ++x) {
            if (cell_on(image.get(x, y), subframe, subframes, levels)) {
                const Address a = map_cell(config, x, y);
                out[a.digit][a.chip] |= static_cast<uint8_t>(1u << a.bit);
            }
        }
    }
}

}  // namespace panel_map
