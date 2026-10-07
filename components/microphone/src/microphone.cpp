// SPDX-License-Identifier: GPL-3.0-or-later
#include "microphone/microphone.hpp"

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"

namespace microphone {

namespace {

constexpr const char* kTag = "microphone";
constexpr int kBlock = 160;  // 10 ms at 16 kHz.

uint32_t now_ms() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000);
}

// The INMP441 sends 24-bit samples, left-aligned in a 32-bit slot.
float to_float(int32_t raw) {
    return static_cast<float>(raw >> 8) / 8388608.0f;
}

}  // namespace

esp_err_t Microphone::start(const MicConfig& config) {
    config_ = config;
    detector_ = beat::BeatDetector(config.sample_rate);
    analyser_ = spectrum::Analyser(config.sample_rate);
    lock_ = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(lock_ != nullptr, ESP_ERR_NO_MEM, kTag, "mutex create failed");

    i2s_chan_config_t channel_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_chan_handle_t channel = nullptr;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&channel_config, nullptr, &channel), kTag,
                        "I2S channel failed");
    channel_ = channel;

    i2s_std_config_t std_config = {};
    std_config.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(static_cast<uint32_t>(config.sample_rate));
    std_config.slot_cfg =
        I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO);
    std_config.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    std_config.gpio_cfg.mclk = I2S_GPIO_UNUSED;
    std_config.gpio_cfg.bclk = static_cast<gpio_num_t>(config.sck_gpio);
    std_config.gpio_cfg.ws = static_cast<gpio_num_t>(config.ws_gpio);
    std_config.gpio_cfg.dout = I2S_GPIO_UNUSED;
    std_config.gpio_cfg.din = static_cast<gpio_num_t>(config.sd_gpio);
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(channel, &std_config), kTag, "I2S init failed");
    // With no microphone, the pull-down holds the data line low, so all samples are 0.
    gpio_pulldown_en(static_cast<gpio_num_t>(config.sd_gpio));
    ESP_RETURN_ON_ERROR(i2s_channel_enable(channel), kTag, "I2S enable failed");

    if (!detect()) {
        ESP_LOGI(kTag, "no microphone found. Beat sync is off.");
        i2s_channel_disable(channel);
        i2s_del_channel(channel);
        channel_ = nullptr;
        return ESP_ERR_NOT_FOUND;
    }
    present_ = true;
    ESP_LOGI(kTag, "microphone found. Beat sync is available.");
    ESP_RETURN_ON_FALSE(xTaskCreate(task_entry, "microphone", 4096, this, 4, nullptr) == pdPASS,
                        ESP_ERR_NO_MEM, kTag, "task create failed");
    return ESP_OK;
}

bool Microphone::detect() {
    // With no microphone, the data line stays at one level, so all samples are the same. A
    // microphone always has some noise. Skip the first samples while the microphone starts.
    int32_t block[kBlock];
    int32_t first = 0;
    bool varies = false;
    for (int i = 0; i < 30 && !varies; ++i) {
        size_t bytes = 0;
        if (i2s_channel_read(static_cast<i2s_chan_handle_t>(channel_), block, sizeof(block), &bytes,
                             pdMS_TO_TICKS(100)) != ESP_OK) {
            return false;
        }
        if (i < 10) {
            continue;
        }
        const int count = static_cast<int>(bytes / sizeof(int32_t));
        for (int k = 0; k < count; ++k) {
            const int32_t value = block[k] >> 8;
            if (i == 10 && k == 0) {
                first = value;
            } else if (value != first) {
                varies = true;
            }
        }
    }
    return varies;
}

void Microphone::set_sensitivity(int sensitivity) {
    xSemaphoreTake(lock_, portMAX_DELAY);
    detector_.set_sensitivity(sensitivity);
    xSemaphoreGive(lock_);
}

bool Microphone::take_beat(uint32_t now) {
    if (!present_) {
        return false;
    }
    xSemaphoreTake(lock_, portMAX_DELAY);
    // Map the esp_timer time to the audio time of the detector.
    const uint32_t audio_ms = detector_.time_ms() + (now - last_block_ms_);
    const bool beat = detector_.take_beat(audio_ms);
    xSemaphoreGive(lock_);
    return beat;
}

float Microphone::bpm() {
    if (!present_) {
        return 0;
    }
    xSemaphoreTake(lock_, portMAX_DELAY);
    const float value = detector_.stable() ? detector_.bpm() : 0.0f;
    xSemaphoreGive(lock_);
    return value;
}

float Microphone::bpm_estimate() {
    if (!present_) {
        return 0;
    }
    xSemaphoreTake(lock_, portMAX_DELAY);
    const float value = detector_.bpm_estimate();
    xSemaphoreGive(lock_);
    return value;
}

bool Microphone::spectrum(float* levels) {
    if (!present_) {
        return false;
    }
    xSemaphoreTake(lock_, portMAX_DELAY);
    const bool fresh = analyser_.updates() != spectrum_seen_ && analyser_.active();
    if (fresh) {
        spectrum_seen_ = analyser_.updates();
        for (int b = 0; b < spectrum::kBands; ++b) {
            levels[b] = analyser_.bands()[b];
        }
    }
    xSemaphoreGive(lock_);
    return fresh;
}

void Microphone::task_entry(void* arg) {
    static_cast<Microphone*>(arg)->run();
}

void Microphone::run() {
    int32_t raw[kBlock];
    float samples[kBlock];
    while (true) {
        size_t bytes = 0;
        if (i2s_channel_read(static_cast<i2s_chan_handle_t>(channel_), raw, sizeof(raw), &bytes,
                             portMAX_DELAY) != ESP_OK) {
            continue;
        }
        const int count = static_cast<int>(bytes / sizeof(int32_t));
        for (int i = 0; i < count; ++i) {
            samples[i] = to_float(raw[i]);
        }
        xSemaphoreTake(lock_, portMAX_DELAY);
        detector_.process(samples, static_cast<size_t>(count));
        analyser_.process(samples, static_cast<size_t>(count));
        last_block_ms_ = now_ms();
        xSemaphoreGive(lock_);
    }
}

}  // namespace microphone
