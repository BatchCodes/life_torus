// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdint>

#include "frame/image.hpp"

namespace frame {

namespace {

// Four bold letters, 8 × 12 cells each. The display shows them twice as large.
constexpr uint8_t kKoGlyphs[4][12] = {
    {0xC3, 0xE3, 0xF3, 0xF3, 0xDB, 0xDB, 0xDB, 0xCF, 0xCF, 0xC7, 0xC3, 0xC3},
    {0xFF, 0xFF, 0xC0, 0xC0, 0xC0, 0xFE, 0xFE, 0xC0, 0xC0, 0xC0, 0xFF, 0xFF},
    {0xFE, 0xFF, 0xC3, 0xC3, 0xC3, 0xFF, 0xFE, 0xD8, 0xCC, 0xC6, 0xC3, 0xC3},
    {0xFC, 0xFE, 0xC7, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC7, 0xFE, 0xFC},
};
constexpr int kScale = 2;
constexpr int kGlyphWidth = 8 * kScale;
constexpr int kGlyphHeight = 12 * kScale;
constexpr int kGap = 6;
constexpr int kTrailingSpace = 24;
constexpr int kStripWidth = 4 * (kGlyphWidth + kGap) - kGap + kTrailingSpace;
constexpr int kTop = (life::kHeight - kGlyphHeight) / 2;

bool strip_cell(int sx, int sy) {
    const int letter = sx / (kGlyphWidth + kGap);
    const int gx = sx % (kGlyphWidth + kGap);
    if (letter >= 4 || gx >= kGlyphWidth || sy < 0 || sy >= kGlyphHeight) {
        return false;
    }
    const uint8_t bits = kKoGlyphs[letter][sy / kScale];
    return (bits >> (7 - gx / kScale)) & 1u;
}

}  // namespace

void draw_ko_code(Image& image, int offset) {
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::width(); ++x) {
            const int sx = ((x + offset) % kStripWidth + kStripWidth) % kStripWidth;
            image.set(x, y, strip_cell(sx, y - kTop) ? Level::kBright : Level::kOff);
        }
    }
}

}  // namespace frame
