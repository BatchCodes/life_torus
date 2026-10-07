// SPDX-License-Identifier: GPL-3.0-or-later
#include "spectrum/analyser.hpp"

#include <algorithm>
#include <cmath>

#include "spectrum/fft.hpp"

namespace spectrum {

namespace {

constexpr float kRangeDb = 40.0f;        // The bars show the loudest 40 dB.
constexpr float kSilenceDb = -75.0f;     // Loudest band below this: silence.
constexpr float kDecayPerSecond = 2.5f;  // Bar height fall, in full heights per second.
constexpr float kGainFallDbPerSecond = 6.0f;

}  // namespace

Analyser::Analyser(int sample_rate) : sample_rate_(sample_rate) {
    // Approximately 30 Hz per FFT bin: 512 points at 16 kHz.
    size_ = 256;
    while (static_cast<int>(size_) * 30 < sample_rate) {
        size_ <<= 1;
    }
    window_.resize(size_);
    input_.assign(size_, 0.0f);
    re_.resize(size_);
    im_.resize(size_);
    for (size_t i = 0; i < size_; ++i) {
        window_[i] = 0.5f - 0.5f * static_cast<float>(std::cos(2.0 * M_PI * i / (size_ - 1)));
    }
    const float bin_hz = static_cast<float>(sample_rate) / static_cast<float>(size_);
    const float high = std::min(kHighHz, static_cast<float>(sample_rate) / 2.0f);
    for (int b = 0; b <= kBands; ++b) {
        const float hz = kLowHz * std::pow(high / kLowHz, static_cast<float>(b) / kBands);
        edges_[b] = static_cast<int>(std::lround(hz / bin_hz));
    }
}

void Analyser::process(const float* samples, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        input_[write_] = samples[i];
        write_ = (write_ + 1) % size_;
        if (++since_update_ == size_ / 2) {
            since_update_ = 0;
            analyse();
        }
    }
}

void Analyser::analyse() {
    for (size_t i = 0; i < size_; ++i) {
        re_[i] = input_[(write_ + i) % size_] * window_[i];
        im_[i] = 0.0f;
    }
    fft(re_.data(), im_.data(), size_);

    const float spectra_per_second = 2.0f * static_cast<float>(sample_rate_) / size_;
    // Scale so that a full-scale sine reads approximately 0 dB.
    const float scale = 4.0f / (static_cast<float>(size_) * static_cast<float>(size_));
    std::array<float, kBands> db{};
    float loudest = -200.0f;
    for (int b = 0; b < kBands; ++b) {
        int first = edges_[b];
        int last = std::max(edges_[b + 1], first + 1);  // A narrow low band uses one bin.
        float power = 0;
        for (int k = first; k < last && k < static_cast<int>(size_ / 2); ++k) {
            power = std::max(power, (re_[k] * re_[k] + im_[k] * im_[k]) * scale);
        }
        db[b] = 10.0f * std::log10(power + 1e-12f);
        loudest = std::max(loudest, db[b]);
    }

    // Automatic gain: follow the loudest band up at once, and down slowly.
    loudest_db_ = std::max(loudest, loudest_db_ - kGainFallDbPerSecond / spectra_per_second);
    if (loudest < kSilenceDb) {
        ++quiet_spectra_;
    } else {
        quiet_spectra_ = 0;
    }
    active_ = quiet_spectra_ < spectra_per_second;

    const float decay = kDecayPerSecond / spectra_per_second;
    for (int b = 0; b < kBands; ++b) {
        float level = active_ ? (db[b] - (loudest_db_ - kRangeDb)) / kRangeDb : 0.0f;
        level = std::clamp(level, 0.0f, 1.0f);
        bands_[b] = level > bands_[b] ? level : std::max(level, bands_[b] - decay);
    }
    ++updates_;
}

}  // namespace spectrum
