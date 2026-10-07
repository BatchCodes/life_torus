// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

#include "esp_err.h"
#include "mode_button/debouncer.hpp"

namespace mode_button {

// A push button from a GPIO to GND, with the internal pull-up. Call poll() often, for example
// every 10 ms.
class ModeButton {
public:
    esp_err_t start(int gpio);
    // Returns true once for each push.
    bool poll(uint32_t now_ms);

private:
    int gpio_ = -1;
    Debouncer debouncer_;
};

}  // namespace mode_button
