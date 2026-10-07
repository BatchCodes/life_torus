// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>

#include "frame/image.hpp"
#include "life/grid.hpp"

namespace display_modes {

constexpr int kMinSpeed = 1;
constexpr int kMaxSpeed = 10;
constexpr int kDefaultSpeed = 5;
constexpr int kMaxTextLength = 120;

// Each effect has a start, a tick that moves it forward in time, and a render. Speed 5 is the
// normal speed, 1 is slow and 10 is fast.
class ScrollingText {
public:
    void start(uint32_t now_ms);
    void set_text(const char* text);
    const char* text() const { return text_; }
    void tick(uint32_t now_ms, int speed);
    void render(frame::Image& image) const;

private:
    char text_[kMaxTextLength + 1] = "Life Torus";
    int length_ = 10;
    uint32_t last_ms_ = 0;
    uint32_t offset_milli_ = 0;  // Scroll position in 1/1000 columns.
};

class Rain {
public:
    void start(uint32_t now_ms, life::Rng& rng);
    void tick(uint32_t now_ms, int speed, life::Rng& rng);
    void render(frame::Image& image) const;

private:
    static constexpr int kDrops = 40;
    struct Drop {
        int8_t x;
        int16_t y16;   // Row of the head in 1/16 rows, so drops fall at different speeds.
        uint8_t rate;  // 1/16 rows per step.
    };
    std::array<Drop, kDrops> drops_{};
    uint32_t last_ms_ = 0;
};

class BarberPole {
public:
    void start(uint32_t now_ms);
    void tick(uint32_t now_ms, int speed);
    void render(frame::Image& image) const;

private:
    uint32_t last_ms_ = 0;
    uint32_t phase_milli_ = 0;
};

class Ripples {
public:
    void start(uint32_t now_ms, life::Rng& rng);
    void tick(uint32_t now_ms, int speed, life::Rng& rng);
    void render(frame::Image& image) const;

private:
    static constexpr int kRipples = 4;
    struct Ripple {
        int8_t x;
        int8_t y;
        int32_t radius_milli;  // -1: not active.
    };
    std::array<Ripple, kRipples> ripples_{};
    uint32_t last_ms_ = 0;
    uint32_t next_spawn_ms_ = 0;
};

class Sparkle {
public:
    void start(uint32_t now_ms, life::Rng& rng);
    void tick(uint32_t now_ms, int speed, life::Rng& rng);
    void render(frame::Image& image) const;

private:
    // Age of each cell in steps: 0 is off, 1 and 2 bright, 3 and 4 normal, 5 and 6 dim.
    std::array<uint8_t, life::kWidth * life::kHeight> age_{};
    uint32_t last_ms_ = 0;
    uint32_t elapsed_ = 0;  // Time since the last step, multiplied by the speed.
};

}  // namespace display_modes
