// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

namespace pixel_map {

// WS2812B panels: 8 columns × 32 rows each, standing upright in the ring. The number of panels
// is the grid width / 8. The panels share up to 4 data lines: each line is a chain of panels.
constexpr int kMaxLines = 4;
constexpr int kPanelColumns = 8;
constexpr int kPanelRows = 32;
constexpr int kLedsPerPanel = kPanelColumns * kPanelRows;

enum class Start : uint8_t { kBottomLeft, kBottomRight, kTopLeft, kTopRight };

// How the LEDs run inside each panel, seen from the front with the panel upright. The bring-up
// firmware finds the right options for a panel type.
struct PixelConfig {
    int lines = 4;                     // Data lines in use, 1 to 4.
    Start start = Start::kBottomLeft;  // The corner with the first LED (the data input).
    bool rows = true;                  // The LEDs run along the rows (8 per row), not the columns.
    bool serpentine = true;            // Every second row (or column) runs back.
    bool zigzag = false;               // Every second panel is turned by 180 degrees.
    bool reverse_ring = false;         // The panel order goes right to left around the ring.
};

struct Led {
    uint8_t line;    // 0 to kMaxLines - 1.
    uint16_t index;  // Position in the chain of that line.
};

int panels();
// The data lines that have at least one panel.
int lines_used(const PixelConfig& config);
// The number of LEDs on a data line.
int line_length(const PixelConfig& config, int line);

Led map_cell(const PixelConfig& config, int x, int y);

}  // namespace pixel_map
