// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>

#include "frame/image.hpp"
#include "life/grid.hpp"

namespace colour {

struct Rgb {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    bool operator==(const Rgb& other) const { return r == other.r && g == other.g && b == other.b; }
};

// An RGB display image: width() × 32 pixels.
class RgbImage {
public:
    Rgb get(int x, int y) const { return pixels_[y * life::kMaxWidth + x]; }
    void set(int x, int y, Rgb rgb) { pixels_[y * life::kMaxWidth + x] = rgb; }

private:
    std::array<Rgb, life::kMaxWidth * life::kHeight> pixels_{};
};

enum class Scheme : uint8_t {
    kColours,  // A colour per cell state in Game of Life, and a colour scheme per display mode.
    kSingle,   // One colour with the brightness levels.
};

// What the image shows, for the colour scheme. The numbers are display_modes::ModeId.
enum class Source : uint8_t {
    kGameOfLife = 0,
    kText = 1,
    kRain = 2,
    kBarberPole = 3,
    kRipples = 4,
    kSparkle = 5,
    kVisualiser = 6,
};

struct ColourConfig {
    Scheme scheme = Scheme::kColours;
    Rgb single{255, 60, 20};           // The single colour.
    uint32_t brightness_percent = 25;  // Scale of all colours, 1 to 100.
    uint32_t current_limit_ma = 4500;  // Largest estimated current of all LEDs.
    // WS2812B values for the current estimate: the current of one colour channel at full
    // scale, and the current of one LED when it is off (in tenths of a mA).
    uint32_t channel_ma = 20;
    uint32_t idle_tenth_ma = 8;
};

Rgb hsv(int hue_degrees, uint8_t value);

// Colours a level image. Highlight cells (the cursor, the ko code effect) are white.
void colourise(const frame::Image& image, Source source, uint32_t now_ms,
               const ColourConfig& config, RgbImage& out);

// The estimated current of the whole display, in mA: all LEDs, also the dark ones.
uint32_t estimate_ma(const RgbImage& image, const ColourConfig& config);

// Scales the image down so that the estimate stays at or under the limit. Returns the scale
// in 1/256 (256: no change).
int limit_current(RgbImage& image, const ColourConfig& config);

}  // namespace colour
