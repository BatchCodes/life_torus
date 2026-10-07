// SPDX-License-Identifier: GPL-3.0-or-later
#include <iterator>

#include "game/game.hpp"

namespace game {

namespace {

constexpr Button kKoCode[] = {
    Button::kUp,    Button::kUp,   Button::kDown,  Button::kDown, Button::kLeft,
    Button::kRight, Button::kLeft, Button::kRight, Button::kB,    Button::kA,
};
constexpr int kKoLength = static_cast<int>(std::size(kKoCode));
// The index of the button whose effect the firmware undoes when the code completes.
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

bool Game::ko_code_press(Button button, uint32_t now_ms) {
    if (next_ko_ > 0 && now_ms - ko_last_ms_ > config_.ko_gap_ms) {
        next_ko_ = 0;
    }
    ko_last_ms_ = now_ms;

    if (button != kKoCode[next_ko_]) {
        const int matched = ko_fallback(next_ko_, button);
        if (matched > 0 && next_ko_ == 0) {
            ko_resume_run_ = mode_ == Mode::kRun;
        }
        next_ko_ = matched;
        return false;
    }

    if (next_ko_ == 0) {
        ko_resume_run_ = mode_ == Mode::kRun;
    }
    if (next_ko_ == kKoUndoFrom) {
        ko_board_ = simulation_.current();
    }
    ++next_ko_;
    if (next_ko_ < kKoLength) {
        return false;
    }

    next_ko_ = 0;
    simulation_.edit(ko_board_);
    dpad_held_ = false;
    mode_ = Mode::kKoCode;
    ko_start_ms_ = now_ms;
    return true;
}

}  // namespace game
