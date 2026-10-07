// SPDX-License-Identifier: GPL-3.0-or-later
#include "board.hpp"

#include "esp_check.h"
#include "sdkconfig.h"

namespace {

constexpr const char* kTag = "board";

}  // namespace

esp_err_t Board::init() {
#ifdef CONFIG_LIFE_PANEL_REVERSE_RING
    panel_.reverse_ring = true;
#endif
#ifdef CONFIG_LIFE_PANEL_FLIP_BOARDS
    panel_.flip_boards = true;
#endif
#ifdef CONFIG_LIFE_PANEL_ZIGZAG
    panel_.zigzag = true;
#endif
#ifdef CONFIG_LIFE_PANEL_BLOCK_TRANSPOSE
    panel_.block_transpose = true;
#endif
#ifdef CONFIG_LIFE_PANEL_BLOCK_FLIP_X
    panel_.block_flip_x = true;
#endif
#ifdef CONFIG_LIFE_PANEL_BLOCK_FLIP_Y
    panel_.block_flip_y = true;
#endif

    max7219_chain::ChainConfig chain_config;
    chain_config.din_gpio = CONFIG_LIFE_PIN_DIN;
    chain_config.clk_gpio = CONFIG_LIFE_PIN_CLK;
    chain_config.cs_gpio = CONFIG_LIFE_PIN_CS;
    chain_config.clock_hz = CONFIG_LIFE_SPI_CLOCK_KHZ * 1000;
    ESP_RETURN_ON_ERROR(chain_.init(chain_config), kTag, "display chain init failed");

    max7219_chain::RefreshConfig refresh_config;
#if CONFIG_LIFE_BRIGHTNESS_LEVELS
    subframes_ = 3;
    refresh_config.subframe_period_ms = CONFIG_LIFE_SUBFRAME_MS;
#else
    subframes_ = 1;
    refresh_config.subframe_period_ms = 10;
#endif
    refresh_config.subframes = subframes_;
    refresh_config.intensity = CONFIG_LIFE_INTENSITY;
    refresh_config.reinit_period_ms = CONFIG_LIFE_REINIT_MS;
    ESP_RETURN_ON_ERROR(refresh_.start(chain_, refresh_config), kTag, "refresh start failed");
    return ESP_OK;
}

void Board::show(const frame::Image& image) {
    for (int s = 0; s < subframes_; ++s) {
        panel_map::encode(panel_, image, s, subframes_, levels(), subframe_data_[s]);
    }
    refresh_.show(subframe_data_);
}

void Board::show_raw(const panel_map::Registers& registers) {
    for (int s = 0; s < subframes_; ++s) {
        subframe_data_[s] = registers;
    }
    refresh_.show(subframe_data_);
}
