// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "esp_err.h"
#include "frame/image.hpp"
#include "max7219_chain/max7219_chain.hpp"
#include "max7219_chain/refresh.hpp"
#include "panel_map/panel_map.hpp"

// The display hardware of the ESP32-S3-DevKitC-1 build, from the Kconfig options.
class Board {
public:
    esp_err_t init();

    // Converts an image to sub-frames and hands them to the refresh task.
    void show(const frame::Image& image);
    // Shows raw registers in every sub-frame.
    void show_raw(const panel_map::Registers& registers);

    panel_map::PanelConfig& panel() { return panel_; }
    void set_intensity(uint8_t intensity) { refresh_.set_intensity(intensity); }
    uint8_t intensity() const { return refresh_.intensity(); }
    int subframes() const { return subframes_; }
    bool levels() const { return subframes_ > 1; }

private:
    max7219_chain::Max7219Chain chain_;
    max7219_chain::Refresh refresh_;
    panel_map::PanelConfig panel_;
    std::array<panel_map::Registers, max7219_chain::kMaxSubframes> subframe_data_{};
    int subframes_ = 1;
};
