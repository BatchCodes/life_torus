// SPDX-License-Identifier: GPL-3.0-or-later
#include "mode_button/debouncer.hpp"

namespace mode_button {

bool Debouncer::update(bool pressed, uint32_t now_ms) {
    if (pressed != candidate_) {
        candidate_ = pressed;
        candidate_since_ms_ = now_ms;
    }
    if (candidate_ == level_ || now_ms - candidate_since_ms_ < stable_ms_) {
        return false;
    }
    level_ = candidate_;
    return level_;
}

}  // namespace mode_button
