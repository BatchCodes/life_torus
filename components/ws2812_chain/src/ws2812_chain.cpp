// SPDX-License-Identifier: GPL-3.0-or-later
#include "ws2812_chain/ws2812_chain.hpp"

#include "esp_check.h"
#include "esp_log.h"

namespace ws2812_chain {

namespace {

constexpr const char* kTag = "ws2812_chain";
constexpr uint32_t kResolutionHz = 10000000;  // 0.1 µs per tick.

}  // namespace

esp_err_t Ws2812Chain::init(const ChainConfig& config, const pixel_map::PixelConfig& layout) {
    layout_ = layout;
    layout_.lines = config.lines;
    lines_ = pixel_map::lines_used(layout_);
    for (int line = 0; line < lines_; ++line) {
        rmt_tx_channel_config_t channel_config = {};
        channel_config.gpio_num = static_cast<gpio_num_t>(config.gpio[line]);
        channel_config.clk_src = RMT_CLK_SRC_DEFAULT;
        channel_config.resolution_hz = kResolutionHz;
        // The ESP32-S3 has 4 TX channels with 48 symbols of memory each.
        channel_config.mem_block_symbols = 48;
        channel_config.trans_queue_depth = 2;
        ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&channel_config, &channels_[line]), kTag,
                            "RMT channel %d failed", line);
        ESP_RETURN_ON_ERROR(new_led_encoder(kResolutionHz, &encoders_[line]), kTag,
                            "encoder failed");
        ESP_RETURN_ON_ERROR(rmt_enable(channels_[line]), kTag, "RMT enable failed");
        buffers_[line].assign(static_cast<size_t>(pixel_map::line_length(layout_, line)) * 3, 0);
    }
    ESP_LOGI(kTag, "%d panels on %d data lines", pixel_map::panels(), lines_);
    return ESP_OK;
}

esp_err_t Ws2812Chain::send() {
    rmt_transmit_config_t transmit = {};
    for (int line = 0; line < lines_; ++line) {
        ESP_RETURN_ON_ERROR(rmt_transmit(channels_[line], encoders_[line], buffers_[line].data(),
                                         buffers_[line].size(), &transmit),
                            kTag, "transmit failed");
    }
    sending_ = true;
    return ESP_OK;
}

esp_err_t Ws2812Chain::show(const colour::RgbImage& image) {
    if (sending_) {
        for (int line = 0; line < lines_; ++line) {
            rmt_tx_wait_all_done(channels_[line], 100);
        }
        sending_ = false;
    }
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::width(); ++x) {
            const pixel_map::Led led = pixel_map::map_cell(layout_, x, y);
            if (led.line >= lines_) {
                continue;
            }
            const colour::Rgb c = image.get(x, y);
            uint8_t* p = buffers_[led.line].data() + static_cast<size_t>(led.index) * 3;
            p[0] = c.g;  // WS2812B colour order: green, red, blue.
            p[1] = c.r;
            p[2] = c.b;
        }
    }
    return send();
}

esp_err_t Ws2812Chain::show_raw(
    const std::array<std::vector<colour::Rgb>, pixel_map::kMaxLines>& leds) {
    if (sending_) {
        for (int line = 0; line < lines_; ++line) {
            rmt_tx_wait_all_done(channels_[line], 100);
        }
        sending_ = false;
    }
    for (int line = 0; line < lines_; ++line) {
        const size_t count = buffers_[line].size() / 3;
        for (size_t i = 0; i < count; ++i) {
            const colour::Rgb c = i < leds[line].size() ? leds[line][i] : colour::Rgb{};
            buffers_[line][i * 3] = c.g;
            buffers_[line][i * 3 + 1] = c.r;
            buffers_[line][i * 3 + 2] = c.b;
        }
    }
    return send();
}

}  // namespace ws2812_chain
