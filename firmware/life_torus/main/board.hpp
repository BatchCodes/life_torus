// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <vector>

#include "colour/colour.hpp"
#include "esp_err.h"
#include "frame/image.hpp"
#include "max7219_chain/max7219_chain.hpp"
#include "max7219_chain/refresh.hpp"
#include "panel_map/panel_map.hpp"
#include "settings/settings.hpp"
#include "ws2812_chain/ws2812_chain.hpp"

// The display hardware of the ESP32-S3-DevKitC-1 build: MAX7219 boards or WS2812B panels, from
// the display setting. The pins come from Kconfig, the layout and the brightness from the
// settings.
class Board {
public:
    esp_err_t init(const settings::Settings& s);

    // Shows an image. A WS2812B display colours it with the scheme of `source`.
    void show(const frame::Image& image, colour::Source source, uint32_t now_ms);
    // MAX7219 only: raw registers in every sub-frame.
    void show_raw(const panel_map::Registers& registers);
    // WS2812B only: raw colours for each LED of each data line.
    void show_raw_leds(const std::array<std::vector<colour::Rgb>, pixel_map::kMaxLines>& leds);

    // Applies the display settings at run time (layout, intensity, brightness, colours).
    void apply(const settings::Settings& s);

    bool ws2812() const { return ws2812_; }
    panel_map::PanelConfig& panel() { return panel_; }
    pixel_map::PixelConfig& pixels() { return pixels_; }
    void set_intensity(uint8_t intensity) { refresh_.set_intensity(intensity); }
    uint8_t intensity() const { return refresh_.intensity(); }
    // WS2812B: the estimated current of the last frame, after the limit.
    uint32_t last_current_ma() const { return last_current_ma_; }

private:
    static constexpr int kSubframes = 3;

    bool ws2812_ = false;
    max7219_chain::Max7219Chain chain_;
    max7219_chain::Refresh refresh_;
    panel_map::PanelConfig panel_;
    bool levels_ = true;
    std::array<panel_map::Registers, max7219_chain::kMaxSubframes> subframe_data_{};

    ws2812_chain::Ws2812Chain pixel_chain_;
    pixel_map::PixelConfig pixels_;
    colour::ColourConfig colours_;
    colour::RgbImage rgb_;
    uint32_t last_current_ma_ = 0;
};
