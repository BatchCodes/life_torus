// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <atomic>
#include <cstdint>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "max7219_chain/max7219_chain.hpp"
#include "panel_map/panel_map.hpp"

namespace max7219_chain {

constexpr int kMaxSubframes = 4;

struct RefreshConfig {
    int subframes = 3;                 // 1: no brightness levels.
    uint32_t subframe_period_ms = 3;   // Time that each sub-frame shows.
    uint32_t reinit_period_ms = 5000;  // Rewrite the start-up registers this often. 0: never.
    uint8_t intensity = 4;
    int task_priority = 5;
    int task_core = 1;
};

// Shows a set of sub-frames in a loop, from its own task. With 3 sub-frames, a cell that is on
// in 1, 2 or 3 sub-frames shows at 1/3, 2/3 or full brightness.
class Refresh {
public:
    esp_err_t start(Max7219Chain& chain, const RefreshConfig& config);
    // Copies a new set of sub-frames. The refresh task shows them from its next loop.
    void show(const std::array<panel_map::Registers, kMaxSubframes>& subframes);
    // Changes the intensity of all chips (0 to 15). The refresh task writes it.
    void set_intensity(uint8_t intensity) { intensity_ = intensity; }
    uint8_t intensity() const { return intensity_; }

private:
    static void task_entry(void* arg);
    void run();

    Max7219Chain* chain_ = nullptr;
    RefreshConfig config_;
    SemaphoreHandle_t lock_ = nullptr;
    std::array<panel_map::Registers, kMaxSubframes> pending_{};
    std::array<panel_map::Registers, kMaxSubframes> active_{};
    std::atomic<bool> has_pending_{false};
    std::atomic<uint8_t> intensity_{4};
};

}  // namespace max7219_chain
