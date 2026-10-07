// SPDX-License-Identifier: GPL-3.0-or-later
//
// WS2812B timing from the WS2812B datasheet (Worldsemi): a 0 bit is 0.4 µs high and 0.85 µs low,
// a 1 bit is 0.8 µs high and 0.45 µs low (±150 ns), and a reset is more than 50 µs low. The
// encoder follows the structure of the ESP-IDF RMT LED strip example: a bytes encoder for the
// colour data, then a copy encoder for the reset pause.
#include <cstdlib>

#include "driver/rmt_encoder.h"
#include "esp_check.h"
#include "ws2812_chain/ws2812_chain.hpp"

namespace ws2812_chain {

namespace {

constexpr const char* kTag = "led_encoder";

struct LedEncoder {
    rmt_encoder_t base;
    rmt_encoder_t* bytes;
    rmt_encoder_t* copy;
    int state;
    rmt_symbol_word_t reset;
};

size_t encode(rmt_encoder_t* encoder, rmt_channel_handle_t channel, const void* data, size_t size,
              rmt_encode_state_t* ret_state) {
    auto* led = reinterpret_cast<LedEncoder*>(encoder);
    rmt_encode_state_t session = RMT_ENCODING_RESET;
    int state = RMT_ENCODING_RESET;
    size_t encoded = 0;
    if (led->state == 0) {
        encoded += led->bytes->encode(led->bytes, channel, data, size, &session);
        if (session & RMT_ENCODING_COMPLETE) {
            led->state = 1;
        }
        if (session & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            *ret_state = static_cast<rmt_encode_state_t>(state);
            return encoded;
        }
    }
    if (led->state == 1) {
        encoded += led->copy->encode(led->copy, channel, &led->reset, sizeof(led->reset), &session);
        if (session & RMT_ENCODING_COMPLETE) {
            led->state = RMT_ENCODING_RESET;
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
        }
    }
    *ret_state = static_cast<rmt_encode_state_t>(state);
    return encoded;
}

esp_err_t remove(rmt_encoder_t* encoder) {
    auto* led = reinterpret_cast<LedEncoder*>(encoder);
    rmt_del_encoder(led->bytes);
    rmt_del_encoder(led->copy);
    std::free(led);
    return ESP_OK;
}

esp_err_t reset(rmt_encoder_t* encoder) {
    auto* led = reinterpret_cast<LedEncoder*>(encoder);
    rmt_encoder_reset(led->bytes);
    rmt_encoder_reset(led->copy);
    led->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

}  // namespace

esp_err_t new_led_encoder(uint32_t resolution_hz, rmt_encoder_handle_t* encoder) {
    auto* led = static_cast<LedEncoder*>(std::calloc(1, sizeof(LedEncoder)));
    ESP_RETURN_ON_FALSE(led != nullptr, ESP_ERR_NO_MEM, kTag, "no memory");
    led->base.encode = encode;
    led->base.del = remove;
    led->base.reset = reset;

    const uint32_t ticks_per_us = resolution_hz / 1000000;
    rmt_bytes_encoder_config_t bytes_config = {};
    bytes_config.bit0.level0 = 1;
    bytes_config.bit0.duration0 = 4 * ticks_per_us / 10;  // 0.4 µs
    bytes_config.bit0.level1 = 0;
    bytes_config.bit0.duration1 = 85 * ticks_per_us / 100;  // 0.85 µs
    bytes_config.bit1.level0 = 1;
    bytes_config.bit1.duration0 = 8 * ticks_per_us / 10;  // 0.8 µs
    bytes_config.bit1.level1 = 0;
    bytes_config.bit1.duration1 = 45 * ticks_per_us / 100;  // 0.45 µs
    bytes_config.flags.msb_first = 1;
    esp_err_t err = rmt_new_bytes_encoder(&bytes_config, &led->bytes);
    if (err == ESP_OK) {
        rmt_copy_encoder_config_t copy_config = {};
        err = rmt_new_copy_encoder(&copy_config, &led->copy);
    }
    if (err != ESP_OK) {
        if (led->bytes != nullptr) {
            rmt_del_encoder(led->bytes);
        }
        std::free(led);
        ESP_RETURN_ON_ERROR(err, kTag, "encoder create failed");
    }
    // Reset: 2 × 25 µs low.
    const uint32_t half = 25 * ticks_per_us;
    led->reset.level0 = 0;
    led->reset.duration0 = half;
    led->reset.level1 = 0;
    led->reset.duration1 = half;
    *encoder = &led->base;
    return ESP_OK;
}

}  // namespace ws2812_chain
