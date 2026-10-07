// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

#include "game/buttons.hpp"

namespace game {

// Follows the button presses and reports when they complete the ko code.
class KoDetector {
public:
    struct Result {
        bool started;   // This press starts a new attempt.
        bool at_undo;   // This press is the first of the last two presses of the code.
        bool complete;  // This press completes the code.
    };

    Result press(Button button, uint32_t now_ms, uint32_t gap_ms);
    void reset() { next_ = 0; }

private:
    int next_ = 0;
    uint32_t last_ms_ = 0;
};

}  // namespace game
