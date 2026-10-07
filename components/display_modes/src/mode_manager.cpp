// SPDX-License-Identifier: GPL-3.0-or-later
#include "display_modes/mode_manager.hpp"

namespace display_modes {

const char* mode_name(ModeId mode) {
    switch (mode) {
        case ModeId::kGameOfLife:
            return "Game of Life";
        case ModeId::kText:
            return "Scrolling text";
        case ModeId::kRain:
            return "Rain";
        case ModeId::kBarberPole:
            return "Barber pole";
        case ModeId::kRipples:
            return "Ripples";
        case ModeId::kSparkle:
            return "Sparkle";
    }
    return "";
}

ModeManager::ModeManager(game::Game& game, uint32_t seed) : game_(game), rng_(seed) {}

void ModeManager::set_speed(int speed) {
    speed_ = speed < kMinSpeed ? kMinSpeed : (speed > kMaxSpeed ? kMaxSpeed : speed);
}

void ModeManager::set_mode(ModeId mode, uint32_t now_ms) {
    if (mode == mode_) {
        return;
    }
    mode_ = mode;
    ko_effect_ = false;
    ko_.reset();
    switch (mode_) {
        case ModeId::kGameOfLife:
            // Continue the board where it stopped, in run.
            game_.tick(now_ms);
            break;
        case ModeId::kText:
            text_.start(now_ms);
            break;
        case ModeId::kRain:
            rain_.start(now_ms, rng_);
            break;
        case ModeId::kBarberPole:
            barber_pole_.start(now_ms);
            break;
        case ModeId::kRipples:
            ripples_.start(now_ms, rng_);
            break;
        case ModeId::kSparkle:
            sparkle_.start(now_ms, rng_);
            break;
    }
}

namespace {

constexpr uint32_t kFlashMs = 120;

frame::Level brighter(frame::Level level) {
    switch (level) {
        case frame::Level::kDim:
            return frame::Level::kNormal;
        case frame::Level::kNormal:
        case frame::Level::kBright:
            return frame::Level::kBright;
        case frame::Level::kOff:
            break;
    }
    return frame::Level::kOff;
}

}  // namespace

bool ModeManager::beats_active(uint32_t now_ms) const {
    return beat_period_ms_ > 0 && now_ms - last_beat_ms_ < 2 * beat_period_ms_;
}

void ModeManager::beat(uint32_t now_ms) {
    // The time since the last beat is the beat period, when it fits 60 to 180 BPM.
    const uint32_t period = now_ms - last_beat_ms_;
    if (period >= 333 && period <= 1000) {
        beat_period_ms_ = period;
    }
    last_beat_ms_ = now_ms;
    switch (mode_) {
        case ModeId::kRain:
            rain_.beat(rng_);
            break;
        case ModeId::kRipples:
            ripples_.beat(rng_);
            break;
        case ModeId::kSparkle:
            sparkle_.beat(rng_);
            break;
        case ModeId::kGameOfLife:
        case ModeId::kText:
        case ModeId::kBarberPole:
            break;
    }
}

void ModeManager::next_mode(uint32_t now_ms) {
    set_mode(static_cast<ModeId>((static_cast<int>(mode_) + 1) % kModeCount), now_ms);
}

void ModeManager::press(game::Button button, uint32_t now_ms) {
    if (mode_ == ModeId::kGameOfLife) {
        game_.press(button, now_ms);
        return;
    }
    if (ko_effect_) {
        return;
    }
    if (ko_.press(button, now_ms, game_.config().ko_gap_ms).complete) {
        ko_effect_ = true;
        ko_start_ms_ = now_ms;
    }
}

void ModeManager::release(game::Button button, uint32_t now_ms) {
    if (mode_ == ModeId::kGameOfLife) {
        game_.release(button, now_ms);
    }
}

void ModeManager::tick(uint32_t now_ms) {
    if (ko_effect_ && now_ms - ko_start_ms_ >= game_.config().ko_effect_ms) {
        ko_effect_ = false;
    }
    switch (mode_) {
        case ModeId::kGameOfLife:
            game_.tick(now_ms);
            break;
        case ModeId::kText:
            text_.set_letter_level(beats_active(now_ms) ? frame::Level::kNormal
                                                        : frame::Level::kBright);
            text_.tick(now_ms, speed_);
            break;
        case ModeId::kRain:
            rain_.tick(now_ms, speed_, rng_);
            break;
        case ModeId::kBarberPole:
            barber_pole_.tick(now_ms, speed_, beats_active(now_ms) ? beat_period_ms_ : 0);
            break;
        case ModeId::kRipples:
            ripples_.tick(now_ms, speed_, rng_, beats_active(now_ms));
            break;
        case ModeId::kSparkle:
            sparkle_.tick(now_ms, speed_, rng_);
            break;
    }
}

void ModeManager::render(frame::Image& image, uint32_t now_ms) const {
    if (ko_effect_) {
        frame::draw_ko_code(
            image, static_cast<int>((now_ms - ko_start_ms_) / game_.config().ko_column_ms));
        return;
    }
    switch (mode_) {
        case ModeId::kGameOfLife:
            game_.render(image, now_ms);
            break;
        case ModeId::kText:
            text_.render(image);
            break;
        case ModeId::kRain:
            rain_.render(image);
            break;
        case ModeId::kBarberPole:
            barber_pole_.render(image);
            break;
        case ModeId::kRipples:
            ripples_.render(image);
            break;
        case ModeId::kSparkle:
            sparkle_.render(image);
            break;
    }
    // Text and barber pole: a short flash on the beat.
    const bool flash_mode = mode_ == ModeId::kText || mode_ == ModeId::kBarberPole;
    if (flash_mode && beats_active(now_ms) && now_ms - last_beat_ms_ < kFlashMs) {
        for (int y = 0; y < life::kHeight; ++y) {
            for (int x = 0; x < life::kWidth; ++x) {
                image.set(x, y, brighter(image.get(x, y)));
            }
        }
    }
}

}  // namespace display_modes
