// SPDX-License-Identifier: GPL-3.0-or-later
#include "pixel_map/pixel_map.hpp"

#include "life/grid.hpp"

namespace pixel_map {

namespace {

// Panels on a line: the panels split in ring order, the first lines get one more when the
// panels do not split evenly. For example 10 panels on 4 lines: 3, 3, 2, 2.
int panels_on_line(const PixelConfig& config, int line) {
    const int count = lines_used(config);
    if (line >= count) {
        return 0;
    }
    return panels() / count + (line < panels() % count ? 1 : 0);
}

}  // namespace

int panels() {
    return life::width() / kPanelColumns;
}

int lines_used(const PixelConfig& config) {
    int lines = config.lines < 1 ? 1 : (config.lines > kMaxLines ? kMaxLines : config.lines);
    return lines < panels() ? lines : panels();
}

int line_length(const PixelConfig& config, int line) {
    return panels_on_line(config, line) * kLedsPerPanel;
}

Led map_cell(const PixelConfig& config, int x, int y) {
    int panel = x / kPanelColumns;
    if (config.reverse_ring) {
        panel = panels() - 1 - panel;
    }
    // Position inside the upright panel: u from the left, v from the top.
    int u = x % kPanelColumns;
    int v = y;
    if (config.zigzag && (panel % 2) == 1) {
        u = kPanelColumns - 1 - u;
        v = kPanelRows - 1 - v;
    }
    const bool right = config.start == Start::kBottomRight || config.start == Start::kTopRight;
    const bool bottom = config.start == Start::kBottomLeft || config.start == Start::kBottomRight;
    const int cu = right ? kPanelColumns - 1 - u : u;  // Columns from the start corner.
    const int cv = bottom ? kPanelRows - 1 - v : v;    // Rows from the start corner.
    int local = 0;
    if (config.rows) {
        const int minor = config.serpentine && (cv % 2) == 1 ? kPanelColumns - 1 - cu : cu;
        local = cv * kPanelColumns + minor;
    } else {
        const int minor = config.serpentine && (cu % 2) == 1 ? kPanelRows - 1 - cv : cv;
        local = cu * kPanelRows + minor;
    }

    // The line of the panel, and the panel's place in that line.
    int line = 0;
    int first = 0;
    while (panel >= first + panels_on_line(config, line)) {
        first += panels_on_line(config, line);
        ++line;
    }
    return Led{static_cast<uint8_t>(line),
               static_cast<uint16_t>((panel - first) * kLedsPerPanel + local)};
}

}  // namespace pixel_map
