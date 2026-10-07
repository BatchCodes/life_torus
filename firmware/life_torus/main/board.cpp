// SPDX-License-Identifier: GPL-3.0-or-later
#include "board.hpp"

#include "esp_check.h"
#include "sdkconfig.h"

namespace {

constexpr const char* kTag = "board";

}  // namespace

esp_err_t Board::init(const settings::Settings& s) {
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
    apply(s);
    ESP_RETURN_ON_ERROR(refresh_.start(chain_, refresh_config), kTag, "refresh start failed");
    return ESP_OK;
}

void Board::apply(const settings::Settings& s) {
    panel_ = s.panel;
    levels_ = s.brightness_levels;
    refresh_.set_intensity(static_cast<uint8_t>(s.intensity));
}

void Board::show(const frame::Image& image) {
    for (int sub = 0; sub < kSubframes; ++sub) {
        panel_map::encode(panel_, image, sub, kSubframes, levels_, subframe_data_[sub]);
    }
    refresh_.show(subframe_data_);
}

void Board::show_raw(const panel_map::Registers& registers) {
    for (int sub = 0; sub < kSubframes; ++sub) {
        subframe_data_[sub] = registers;
    }
    refresh_.show(subframe_data_);
}
