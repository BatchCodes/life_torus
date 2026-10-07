// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>

#include "beat/beat_detector.hpp"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "spectrum/analyser.hpp"

namespace microphone {

struct MicConfig {
    int sck_gpio = 4;
    int ws_gpio = 5;
    int sd_gpio = 6;
    int sample_rate = 16000;
};

// An optional INMP441 I2S microphone (L/R pin to GND: left channel). A task reads it and feeds
// the beat detector. start() returns ESP_ERR_NOT_FOUND when no microphone answers.
class Microphone {
public:
    Microphone() : detector_(16000), analyser_(16000) {}

    esp_err_t start(const MicConfig& config);
    bool present() const { return present_; }

    void set_sensitivity(int sensitivity);
    // True once for each beat at or before `now_ms` (esp_timer time), while the beat is stable.
    bool take_beat(uint32_t now_ms);
    // The tempo, or 0 when there is no stable beat.
    float bpm();
    // The running tempo estimate, also before the beat is stable, or 0.
    float bpm_estimate();
    // Where the stable beat comes from: bass, full range, or none.
    beat::Source source();
    // Copies the spectrum when a new one is ready and there is sound. Returns false otherwise.
    bool spectrum(float* levels);

private:
    static void task_entry(void* arg);
    void run();
    bool detect();

    MicConfig config_;
    void* channel_ = nullptr;  // i2s_chan_handle_t
    beat::BeatDetector detector_;
    spectrum::Analyser analyser_;
    uint32_t spectrum_seen_ = 0;
    SemaphoreHandle_t lock_ = nullptr;
    uint32_t last_block_ms_ = 0;  // esp_timer time of the last block.
    bool present_ = false;
};

}  // namespace microphone
