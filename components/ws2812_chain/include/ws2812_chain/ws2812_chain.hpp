// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "colour/colour.hpp"
#include "driver/rmt_tx.h"
#include "esp_err.h"
#include "pixel_map/pixel_map.hpp"

namespace ws2812_chain {

struct ChainConfig {
    std::array<int, pixel_map::kMaxLines> gpio = {11, 12, 10, 13};
    int lines = 4;
};

// WS2812B panels on up to 4 data lines. Each line is an RMT transmit channel, and all lines send
// at the same time. A frame of 3 panels per line (768 LEDs) takes approximately 23 ms.
class Ws2812Chain {
public:
    esp_err_t init(const ChainConfig& config, const pixel_map::PixelConfig& layout);
    void set_layout(const pixel_map::PixelConfig& layout) { layout_ = layout; }
    // Sends an RGB image. Waits for the previous frame first.
    esp_err_t show(const colour::RgbImage& image);
    // Sends raw GRB data for each LED of each line, for the bring-up test patterns.
    esp_err_t show_raw(const std::array<std::vector<colour::Rgb>, pixel_map::kMaxLines>& leds);

private:
    esp_err_t send();

    pixel_map::PixelConfig layout_;
    int lines_ = 0;
    std::array<rmt_channel_handle_t, pixel_map::kMaxLines> channels_{};
    std::array<rmt_encoder_handle_t, pixel_map::kMaxLines> encoders_{};
    std::array<std::vector<uint8_t>, pixel_map::kMaxLines> buffers_;  // GRB bytes per line.
    bool sending_ = false;
};

// An RMT encoder for WS2812B: 800 kHz bits, then a reset pause of 50 µs.
esp_err_t new_led_encoder(uint32_t resolution_hz, rmt_encoder_handle_t* encoder);

}  // namespace ws2812_chain
