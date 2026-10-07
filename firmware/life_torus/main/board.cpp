// SPDX-License-Identifier: GPL-3.0-or-later
#include "board.hpp"

#include "esp_check.h"
#include "sdkconfig.h"

namespace {

constexpr const char* kTag = "board";

}  // namespace

esp_err_t Board::init(const settings::Settings& s) {
    ws2812_ = s.ws2812;
    apply(s);
    if (ws2812_) {
        ws2812_chain::ChainConfig config;
        config.gpio = {CONFIG_LIFE_PIN_WS_LINE1, CONFIG_LIFE_PIN_WS_LINE2, CONFIG_LIFE_PIN_WS_LINE3,
                       CONFIG_LIFE_PIN_WS_LINE4};
        config.lines = CONFIG_LIFE_WS2812_LINES;
        ESP_RETURN_ON_ERROR(pixel_chain_.init(config, pixels_), kTag, "WS2812B init failed");
        return ESP_OK;
    }

    max7219_chain::ChainConfig chain_config;
    chain_config.din_gpio = CONFIG_LIFE_PIN_DIN;
    chain_config.clk_gpio = CONFIG_LIFE_PIN_CLK;
    chain_config.cs_gpio = CONFIG_LIFE_PIN_CS;
    chain_config.clock_hz = CONFIG_LIFE_SPI_CLOCK_KHZ * 1000;
    ESP_RETURN_ON_ERROR(chain_.init(chain_config), kTag, "display chain init failed");

    // Always 3 sub-frames. With brightness levels off, all 3 are the same, so nothing flickers.
    max7219_chain::RefreshConfig refresh_config;
    refresh_config.subframes = kSubframes;
    refresh_config.subframe_period_ms = CONFIG_LIFE_SUBFRAME_MS;
    refresh_config.intensity = static_cast<uint8_t>(s.intensity);
    refresh_config.reinit_period_ms = CONFIG_LIFE_REINIT_MS;
    ESP_RETURN_ON_ERROR(refresh_.start(chain_, refresh_config), kTag, "refresh start failed");
    return ESP_OK;
}

void Board::apply(const settings::Settings& s) {
    panel_ = s.panel;
    levels_ = s.brightness_levels;
    pixels_ = s.pixels;
    pixels_.lines = CONFIG_LIFE_WS2812_LINES;
    pixel_chain_.set_layout(pixels_);
    colours_.scheme = s.single_colour ? colour::Scheme::kSingle : colour::Scheme::kColours;
    colours_.single =
        colour::Rgb{static_cast<uint8_t>(s.colour >> 16), static_cast<uint8_t>(s.colour >> 8),
                    static_cast<uint8_t>(s.colour)};
    colours_.brightness_percent = s.brightness;
    colours_.current_limit_ma = s.led_current_ma;
    if (!ws2812_) {
        refresh_.set_intensity(static_cast<uint8_t>(s.intensity));
    }
}

void Board::show(const frame::Image& image, colour::Source source, uint32_t now_ms) {
    if (ws2812_) {
        colour::colourise(image, source, now_ms, colours_, rgb_);
        colour::limit_current(rgb_, colours_);
        last_current_ma_ = colour::estimate_ma(rgb_, colours_);
        pixel_chain_.show(rgb_);
        return;
    }
    for (int sub = 0; sub < kSubframes; ++sub) {
        panel_map::encode(panel_, image, sub, kSubframes, levels_, subframe_data_[sub]);
    }
    refresh_.show(subframe_data_);
}

void Board::show_raw(const panel_map::Registers& registers) {
    if (ws2812_) {
        return;
    }
    for (int sub = 0; sub < kSubframes; ++sub) {
        subframe_data_[sub] = registers;
    }
    refresh_.show(subframe_data_);
}

void Board::show_raw_leds(const std::array<std::vector<colour::Rgb>, pixel_map::kMaxLines>& leds) {
    if (ws2812_) {
        pixel_chain_.show_raw(leds);
    }
}
