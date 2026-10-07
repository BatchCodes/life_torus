// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/ko_code.hpp"

#include <iterator>

#include "game/game.hpp"

namespace game {

namespace {

constexpr Button kKoCode[] = {
    Button::kUp,    Button::kUp,   Button::kDown,  Button::kDown, Button::kLeft,
    Button::kRight, Button::kLeft, Button::kRight, Button::kB,    Button::kA,
};
constexpr int kKoLength = static_cast<int>(std::size(kKoCode));
// The index of the button whose effect the game undoes when the code completes.
constexpr int kKoUndoFrom = kKoLength - 2;

// After a wrong press: the longest start of the code that the last presses still match.
int ko_fallback(int matched, Button button) {
    for (int k = matched; k >= 1; --k) {
        // The presses are kKoCode[matched - k + 1 .. matched - 1] followed by `button`.
        if (kKoCode[k - 1] != button) {
            continue;
        }
        bool same = true;
        for (int i = 0; i < k - 1; ++i) {
            if (kKoCode[i] != kKoCode[matched - k + 1 + i]) {
                same = false;
                break;
            }
        }
        if (same) {
            return k;
        }
    }
    return 0;
}

}  // namespace

KoDetector::Result KoDetector::press(Button button, uint32_t now_ms, uint32_t gap_ms) {
    if (next_ > 0 && now_ms - last_ms_ > gap_ms) {
        next_ = 0;
    }
    last_ms_ = now_ms;

    Result result{false, false, false};
    if (button != kKoCode[next_]) {
        const int matched = ko_fallback(next_, button);
        result.started = matched > 0 && next_ == 0;
        next_ = matched;
        return result;
    }
    result.started = next_ == 0;
    result.at_undo = next_ == kKoUndoFrom;
    ++next_;
    if (next_ == kKoLength) {
        next_ = 0;
        result.complete = true;
    }
    return result;
}

bool Game::ko_code_press(Button button, uint32_t now_ms) {
    const KoDetector::Result result = ko_.press(button, now_ms, config_.ko_gap_ms);
    if (result.started) {
        ko_resume_run_ = mode_ == Mode::kRun;
    }
    if (result.at_undo) {
        ko_board_ = simulation_.current();
    }
    if (!result.complete) {
        return false;
    }
    simulation_.edit(ko_board_);
    dpad_held_ = false;
    mode_ = Mode::kKoCode;
    ko_start_ms_ = now_ms;
    return true;
}

}  // namespace game
