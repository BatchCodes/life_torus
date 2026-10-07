// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace spectrum {

constexpr int kBands = 32;
constexpr float kLowHz = 40.0f;
constexpr float kHighHz = 8000.0f;

// Turns audio into 32 band levels for a spectrum display. The bands are spaced on a log scale
// from 40 Hz to 8 kHz. The levels are 0 to 1, with an automatic gain, so quiet and loud music
// both use the full height. A new spectrum comes every half FFT length.
class Analyser {
public:
    explicit Analyser(int sample_rate);

    // Processes samples in the range -1 to 1.
    void process(const float* samples, size_t count);

    const std::array<float, kBands>& bands() const { return bands_; }
    // The number of spectra since the start. Changes when the bands change.
    uint32_t updates() const { return updates_; }
    // False after approximately 1 s of silence.
    bool active() const { return active_; }
    int fft_size() const { return static_cast<int>(size_); }

private:
    void analyse();

    int sample_rate_;
    size_t size_;
    std::vector<float> window_;
    std::vector<float> input_;  // Ring buffer of the last `size_` samples.
    size_t write_ = 0;          // Next position in `input_`, so the oldest sample.
    std::vector<float> re_;
    std::vector<float> im_;
    size_t since_update_ = 0;
    std::array<int, kBands + 1> edges_{};  // First FFT bin of each band.
    std::array<float, kBands> bands_{};
    float loudest_db_ = -60.0f;
    int quiet_spectra_ = 1000;
    bool active_ = false;
    uint32_t updates_ = 0;
};

}  // namespace spectrum
