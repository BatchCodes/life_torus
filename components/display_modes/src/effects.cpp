// SPDX-License-Identifier: GPL-3.0-or-later
#include "display_modes/effects.hpp"

#include <cmath>
#include <cstring>

#include "display_modes/font.hpp"

namespace display_modes {

namespace {

using frame::Level;

constexpr int kTextTop = (life::kHeight - kCharHeight) / 2;
constexpr int kTextGap = life::kWidth;  // Empty columns after the message.

uint32_t elapsed(uint32_t& last_ms, uint32_t now_ms) {
    const uint32_t dt = now_ms - last_ms;
    last_ms = now_ms;
    // Limit a long gap (for example after a mode change) to one normal frame.
    return dt > 200 ? 20 : dt;
}

}  // namespace

// ----- Scrolling text: 4 × speed columns per second.

void ScrollingText::start(uint32_t now_ms) {
    last_ms_ = now_ms;
    offset_milli_ = 0;
}

void ScrollingText::set_text(const char* text) {
    std::strncpy(text_, text, kMaxTextLength);
    text_[kMaxTextLength] = '\0';
    length_ = static_cast<int>(std::strlen(text_));
    offset_milli_ = 0;
}

void ScrollingText::tick(uint32_t now_ms, int speed) {
    offset_milli_ += elapsed(last_ms_, now_ms) * 4u * static_cast<uint32_t>(speed);
    const uint32_t strip = static_cast<uint32_t>(length_ * kCharAdvance + kTextGap) * 1000u;
    offset_milli_ %= strip;
}

void ScrollingText::render(frame::Image& image) const {
    image.fill(Level::kOff);
    const int strip = length_ * kCharAdvance + kTextGap;
    const int offset = static_cast<int>(offset_milli_ / 1000u);
    for (int x = 0; x < life::kWidth; ++x) {
        const int sx = (x + offset) % strip;
        const int index = sx / kCharAdvance;
        if (index >= length_) {
            continue;
        }
        for (int y = 0; y < kCharHeight; ++y) {
            if (large_char_cell(text_[index], sx % kCharAdvance, y)) {
                image.set(x, kTextTop + y, letter_level_);
            }
        }
    }
}

// ----- Rain: drops with a bright head and a fading trail.

namespace {

void new_drop(int8_t& x, int16_t& y16, uint8_t& rate, life::Rng& rng, bool anywhere) {
    x = static_cast<int8_t>(rng.below(life::kWidth));
    const int start_row = anywhere ? static_cast<int>(rng.below(life::kHeight))
                                   : -static_cast<int>(rng.below(life::kHeight));
    y16 = static_cast<int16_t>(start_row * 16);
    rate = static_cast<uint8_t>(4 + rng.below(9));
}

}  // namespace

void Rain::start(uint32_t now_ms, life::Rng& rng) {
    last_ms_ = now_ms;
    for (Drop& d : drops_) {
        new_drop(d.x, d.y16, d.rate, rng, true);
    }
}

void Rain::tick(uint32_t now_ms, int speed, life::Rng& rng) {
    const uint32_t dt = elapsed(last_ms_, now_ms);
    for (Drop& d : drops_) {
        d.y16 = static_cast<int16_t>(d.y16 + d.rate * dt * static_cast<uint32_t>(speed) / 200u);
        if (d.y16 / 16 > life::kHeight + 4) {
            new_drop(d.x, d.y16, d.rate, rng, false);
        }
    }
}

void Rain::beat(life::Rng& rng) {
    // Restart a quarter of the drops at the top, at once.
    for (int i = 0; i < kDrops / 4; ++i) {
        Drop& d = drops_[rng.below(kDrops)];
        d.x = static_cast<int8_t>(rng.below(life::kWidth));
        d.y16 = 0;
        d.rate = static_cast<uint8_t>(8 + rng.below(5));
    }
}

void Rain::render(frame::Image& image) const {
    image.fill(Level::kOff);
    constexpr Level kTrail[4] = {Level::kBright, Level::kNormal, Level::kDim, Level::kDim};
    for (const Drop& d : drops_) {
        const int head = d.y16 >= 0 ? d.y16 / 16 : -1 - (-d.y16 - 1) / 16;
        for (int i = 0; i < 4; ++i) {
            const int y = head - i;
            if (y >= 0 && y < life::kHeight && image.get(d.x, y) < kTrail[i]) {
                image.set(d.x, y, kTrail[i]);
            }
        }
    }
}

// ----- Barber pole: diagonal stripes that turn around the ring.

void BarberPole::start(uint32_t now_ms) {
    last_ms_ = now_ms;
    phase_milli_ = 0;
}

void BarberPole::tick(uint32_t now_ms, int speed, uint32_t beat_period_ms) {
    const uint32_t dt = elapsed(last_ms_, now_ms);
    if (beat_period_ms > 0) {
        // One stripe (8 columns) per beat.
        phase_milli_ += dt * 8000u / beat_period_ms;
    } else {
        // Speed 5: one column every 80 ms.
        phase_milli_ += dt * static_cast<uint32_t>(speed) * 5u / 2u;
    }
    phase_milli_ %= 8000u;
}

void BarberPole::render(frame::Image& image) const {
    constexpr Level kStripe[8] = {Level::kBright, Level::kBright, Level::kNormal, Level::kNormal,
                                  Level::kDim,    Level::kOff,    Level::kOff,    Level::kOff};
    const int phase = static_cast<int>(phase_milli_ / 1000u);
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::kWidth; ++x) {
            image.set(x, y, kStripe[(x + y + 8 - phase) % 8]);
        }
    }
}

// ----- Ripples: rings that spread from random points, across the seam.

void Ripples::start(uint32_t now_ms, life::Rng& rng) {
    last_ms_ = now_ms;
    next_spawn_ms_ = now_ms;
    for (Ripple& r : ripples_) {
        r.radius_milli = -1;
    }
    (void)rng;
}

void Ripples::beat(life::Rng& rng) {
    for (Ripple& r : ripples_) {
        if (r.radius_milli < 0) {
            r.x = static_cast<int8_t>(rng.below(life::kWidth));
            r.y = static_cast<int8_t>(rng.below(life::kHeight));
            r.radius_milli = 0;
            return;
        }
    }
}

void Ripples::tick(uint32_t now_ms, int speed, life::Rng& rng, bool beats) {
    const uint32_t dt = elapsed(last_ms_, now_ms);
    for (Ripple& r : ripples_) {
        if (r.radius_milli < 0) {
            continue;
        }
        // Speed 5: 12 cells per second.
        r.radius_milli += static_cast<int32_t>(dt * static_cast<uint32_t>(speed) * 12u / 5u);
        if (r.radius_milli > 40000) {
            r.radius_milli = -1;
        }
    }
    if (!beats && static_cast<int32_t>(now_ms - next_spawn_ms_) >= 0) {
        next_spawn_ms_ = now_ms + 3000u / static_cast<uint32_t>(speed);
        for (Ripple& r : ripples_) {
            if (r.radius_milli < 0) {
                r.x = static_cast<int8_t>(rng.below(life::kWidth));
                r.y = static_cast<int8_t>(rng.below(life::kHeight));
                r.radius_milli = 0;
                break;
            }
        }
    }
}

void Ripples::render(frame::Image& image) const {
    image.fill(Level::kOff);
    for (const Ripple& r : ripples_) {
        if (r.radius_milli < 0) {
            continue;
        }
        const float radius = static_cast<float>(r.radius_milli) / 1000.0f;
        for (int y = 0; y < life::kHeight; ++y) {
            int dy = std::abs(y - r.y);
            dy = dy < life::kHeight - dy ? dy : life::kHeight - dy;
            for (int x = 0; x < life::kWidth; ++x) {
                int dx = std::abs(x - r.x);
                dx = dx < life::kWidth - dx ? dx : life::kWidth - dx;
                const float band =
                    std::fabs(std::sqrt(static_cast<float>(dx * dx + dy * dy)) - radius);
                const Level level = band < 0.75f  ? Level::kBright
                                    : band < 1.5f ? Level::kNormal
                                    : band < 2.5f ? Level::kDim
                                                  : Level::kOff;
                if (image.get(x, y) < level) {
                    image.set(x, y, level);
                }
            }
        }
    }
}

// ----- Sparkle: random cells that light up and fade.

void Sparkle::start(uint32_t now_ms, life::Rng& rng) {
    last_ms_ = now_ms;
    elapsed_ = 0;
    age_.fill(0);
    (void)rng;
}

void Sparkle::tick(uint32_t now_ms, int speed, life::Rng& rng) {
    // Speed 5: one step every 60 ms.
    constexpr uint32_t kStep = 60u * kDefaultSpeed;
    elapsed_ += elapsed(last_ms_, now_ms) * static_cast<uint32_t>(speed);
    while (elapsed_ >= kStep) {
        elapsed_ -= kStep;
        for (uint8_t& age : age_) {
            if (age > 0) {
                age = age >= 6 ? 0 : static_cast<uint8_t>(age + 1);
            }
        }
        for (int i = 0; i < 24; ++i) {
            age_[rng.below(life::kWidth * life::kHeight)] = 1;
        }
    }
}

void Sparkle::beat(life::Rng& rng) {
    for (int i = 0; i < 160; ++i) {
        age_[rng.below(life::kWidth * life::kHeight)] = 1;
    }
}

void Sparkle::render(frame::Image& image) const {
    constexpr Level kByAge[7] = {Level::kOff,    Level::kBright, Level::kBright, Level::kNormal,
                                 Level::kNormal, Level::kDim,    Level::kDim};
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::kWidth; ++x) {
            image.set(x, y, kByAge[age_[y * life::kWidth + x]]);
        }
    }
}

// ----- Visualiser: a spectrum of the music, or a slow wave with no music.

void Visualiser::start(uint32_t now_ms) {
    last_ms_ = now_ms;
    peaks_.fill(0);
}

void Visualiser::set_bands(const float* levels, uint32_t now_ms) {
    for (int b = 0; b < kBands; ++b) {
        levels_[b] = levels[b] < 0 ? 0 : (levels[b] > 1 ? 1 : levels[b]);
    }
    last_bands_ms_ = now_ms;
    has_bands_ = true;
}

void Visualiser::tick(uint32_t now_ms, int speed) {
    const uint32_t dt = elapsed(last_ms_, now_ms);
    idle_ = !has_bands_ || now_ms - last_bands_ms_ > 1000;
    if (idle_) {
        // Speed 5: one wave in approximately 4 s.
        wave_ms_ += dt * static_cast<uint32_t>(speed);
        const float t = static_cast<float>(wave_ms_) / 5000.0f * 1.6f;
        for (int b = 0; b < kBands; ++b) {
            levels_[b] = 0.35f + 0.25f * std::sin(t + static_cast<float>(b) * 0.39f) +
                         0.1f * std::sin(t * 1.7f - static_cast<float>(b) * 0.2f);
        }
    }
    // The peak markers fall at 0.6 full heights per second.
    const float fall = 0.6f * static_cast<float>(dt) / 1000.0f;
    for (int b = 0; b < kBands; ++b) {
        peaks_[b] = levels_[b] > peaks_[b] ? levels_[b] : peaks_[b] - fall;
    }
}

void Visualiser::render(frame::Image& image) const {
    image.fill(Level::kOff);
    for (int b = 0; b < kBands; ++b) {
        const int height = static_cast<int>(levels_[b] * life::kHeight + 0.5f);
        const int peak = static_cast<int>(peaks_[b] * life::kHeight + 0.5f);
        for (int c = 0; c < 2; ++c) {
            const int x = b * 2 + c;
            for (int h = 0; h < height; ++h) {
                image.set(x, life::kHeight - 1 - h,
                          h == height - 1 ? Level::kBright : Level::kNormal);
            }
            if (peak > height && peak <= life::kHeight) {
                image.set(x, life::kHeight - peak, Level::kDim);
            }
        }
    }
}

}  // namespace display_modes
