// SPDX-License-Identifier: GPL-3.0-or-later
#include "colour/colour.hpp"

namespace colour {

namespace {

using frame::Level;

// The share of full brightness for each level, in 1/256.
int level_scale(Level level) {
    switch (level) {
        case Level::kOff:
            return 0;
        case Level::kDim:
            return 48;
        case Level::kNormal:
            return 128;
        case Level::kBright:
        case Level::kHighlight:
            return 256;
    }
    return 0;
}

Rgb scale(Rgb c, int amount) {
    return Rgb{static_cast<uint8_t>(c.r * amount / 256), static_cast<uint8_t>(c.g * amount / 256),
               static_cast<uint8_t>(c.b * amount / 256)};
}

// A small hash, for sparkle colours that stay with a cell.
uint32_t mix(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

Rgb mode_colour(Source source, int x, int y, uint32_t now_ms) {
    const int width = life::width();
    switch (source) {
        case Source::kGameOfLife:
            return Rgb{255, 255, 255};  // Not used: Game of Life colours come per level.
        case Source::kText:
            return hsv(static_cast<int>((now_ms / 40) % 360), 255);  // A slow colour change.
        case Source::kRain:
            return hsv(x * 360 / width, 255);  // A rainbow around the ring.
        case Source::kBarberPole:
            return hsv(static_cast<int>(((x + y) * 360 / 16 + now_ms / 20) % 360), 255);
        case Source::kRipples:
            return hsv(static_cast<int>((now_ms / 60 + y * 4) % 360), 255);
        case Source::kSparkle:
            return hsv(static_cast<int>(
                           mix(static_cast<uint32_t>(x * 131 + y * 7919) + now_ms / 3000) % 360),
                       255);
        case Source::kVisualiser:
            return hsv(x * 270 / width, 255);  // Red bass to violet treble.
    }
    return Rgb{255, 255, 255};
}

Rgb life_colour(Level level) {
    switch (level) {
        case Level::kBright:
            return Rgb{40, 255, 60};  // Born: green.
        case Level::kNormal:
            return Rgb{40, 90, 255};  // Survives: blue.
        case Level::kDim:
            return Rgb{255, 40, 20};  // Dies next: red.
        case Level::kHighlight:
            return Rgb{255, 255, 255};
        case Level::kOff:
            break;
    }
    return Rgb{};
}

}  // namespace

Rgb hsv(int hue_degrees, uint8_t value) {
    const int h = ((hue_degrees % 360) + 360) % 360;
    const int sector = h / 60;
    const int f = (h % 60) * 255 / 60;
    const uint8_t v = value;
    const uint8_t rising = static_cast<uint8_t>(v * f / 255);
    const uint8_t falling = static_cast<uint8_t>(v * (255 - f) / 255);
    switch (sector) {
        case 0:
            return Rgb{v, rising, 0};
        case 1:
            return Rgb{falling, v, 0};
        case 2:
            return Rgb{0, v, rising};
        case 3:
            return Rgb{0, falling, v};
        case 4:
            return Rgb{rising, 0, v};
        default:
            return Rgb{v, 0, falling};
    }
}

void colourise(const frame::Image& image, Source source, uint32_t now_ms,
               const ColourConfig& config, RgbImage& out) {
    const int brightness =
        static_cast<int>(config.brightness_percent > 100 ? 100 : config.brightness_percent);
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::width(); ++x) {
            const Level level = image.get(x, y);
            Rgb c{};
            if (level == Level::kHighlight) {
                c = Rgb{255, 255, 255};
            } else if (level == Level::kOff) {
                c = Rgb{};
            } else if (config.scheme == Scheme::kSingle) {
                c = scale(config.single, level_scale(level));
            } else if (source == Source::kGameOfLife) {
                c = life_colour(level);
            } else {
                c = scale(mode_colour(source, x, y, now_ms), level_scale(level));
            }
            out.set(x, y, scale(c, brightness * 256 / 100));
        }
    }
}

uint32_t estimate_ma(const RgbImage& image, const ColourConfig& config) {
    uint64_t channel_sum = 0;
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::width(); ++x) {
            const Rgb c = image.get(x, y);
            channel_sum += c.r + c.g + c.b;
        }
    }
    const uint64_t leds = static_cast<uint64_t>(life::width()) * life::kHeight;
    return static_cast<uint32_t>(leds * config.idle_tenth_ma / 10 +
                                 channel_sum * config.channel_ma / 255);
}

int limit_current(RgbImage& image, const ColourConfig& config) {
    const uint64_t leds = static_cast<uint64_t>(life::width()) * life::kHeight;
    const uint32_t idle = static_cast<uint32_t>(leds * config.idle_tenth_ma / 10);
    const uint32_t total = estimate_ma(image, config);
    if (total <= config.current_limit_ma) {
        return 256;
    }
    const uint32_t lit = total - idle;
    const uint32_t room = config.current_limit_ma > idle ? config.current_limit_ma - idle : 0;
    const int amount = static_cast<int>(static_cast<uint64_t>(room) * 256 / lit);
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::width(); ++x) {
            image.set(x, y, scale(image.get(x, y), amount));
        }
    }
    return amount;
}

}  // namespace colour
