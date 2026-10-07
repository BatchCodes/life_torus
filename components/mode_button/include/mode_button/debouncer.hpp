// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

namespace mode_button {

// Turns the bouncing contact of a push button into one press per push. The input must stay at
// a new level for `stable_ms` before the debouncer accepts it.
class Debouncer {
public:
    explicit Debouncer(uint32_t stable_ms = 30) : stable_ms_(stable_ms) {}

    // Returns true once, when the button becomes pressed.
    bool update(bool pressed, uint32_t now_ms);
    bool pressed() const { return level_; }

private:
    uint32_t stable_ms_;
    bool level_ = false;
    bool candidate_ = false;
    uint32_t candidate_since_ms_ = 0;
};

}  // namespace mode_button
