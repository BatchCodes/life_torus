// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

#include "display_modes/effects.hpp"
#include "frame/image.hpp"
#include "game/game.hpp"
#include "game/ko_code.hpp"
#include "life/grid.hpp"

namespace display_modes {

enum class ModeId : uint8_t {
    kGameOfLife,
    kText,
    kRain,
    kBarberPole,
    kRipples,
    kSparkle,
    kVisualiser,
};
constexpr int kModeCount = 7;

const char* mode_name(ModeId mode);

// Chooses what the display shows. Power on starts Game of Life. Another mode stays on until
// set_mode() changes it. In another mode, the controller buttons do nothing, except the ko code.
class ModeManager {
public:
    ModeManager(game::Game& game, uint32_t seed);

    void set_mode(ModeId mode, uint32_t now_ms);
    // The mode button: Game of Life, text, rain, barber pole, ripples, sparkle, then again.
    void next_mode(uint32_t now_ms);
    ModeId mode() const { return mode_; }
    void set_text(const char* text) { text_.set_text(text); }
    const char* text() const { return text_.text(); }
    void set_speed(int speed);
    int speed() const { return speed_; }
    bool ko_effect_on() const { return ko_effect_; }

    // A beat of the music, from the beat detector. Game of Life ignores it. A mode reacts to the
    // beats only while they keep coming.
    void beat(uint32_t now_ms);
    // The music spectrum for the visualiser: kBands levels from 0 to 1, bass first. Call it at
    // each new spectrum while there is sound.
    void set_spectrum(const float* levels, uint32_t now_ms) {
        visualiser_.set_bands(levels, now_ms);
    }
    bool beats_active(uint32_t now_ms) const;

    void press(game::Button button, uint32_t now_ms);
    void release(game::Button button, uint32_t now_ms);
    void tick(uint32_t now_ms);
    void render(frame::Image& image, uint32_t now_ms) const;

private:
    game::Game& game_;
    life::Rng rng_;
    ModeId mode_ = ModeId::kGameOfLife;
    int speed_ = kDefaultSpeed;
    ScrollingText text_;
    Rain rain_;
    BarberPole barber_pole_;
    Ripples ripples_;
    Sparkle sparkle_;
    Visualiser visualiser_;
    uint32_t last_beat_ms_ = 0;
    uint32_t beat_period_ms_ = 0;  // 0: no beats yet.
    game::KoDetector ko_;
    bool ko_effect_ = false;
    uint32_t ko_start_ms_ = 0;
};

}  // namespace display_modes
