// SPDX-License-Identifier: GPL-3.0-or-later
#include "beat/beat_detector.hpp"

#include <cmath>

namespace beat {

namespace {

constexpr float kPi = 3.14159265f;

// Second-order sections from the Audio EQ Cookbook (R. Bristow-Johnson), Q = 0.707.
void design(float* b, float* a, float cutoff_hz, int sample_rate, bool high_pass) {
    const float w = 2.0f * kPi * cutoff_hz / static_cast<float>(sample_rate);
    const float alpha = std::sin(w) / (2.0f * 0.7071f);
    const float c = std::cos(w);
    const float a0 = 1.0f + alpha;
    if (high_pass) {
        b[0] = (1.0f + c) / 2.0f / a0;
        b[1] = -(1.0f + c) / a0;
    } else {
        b[0] = (1.0f - c) / 2.0f / a0;
        b[1] = (1.0f - c) / a0;
    }
    b[2] = b[0];
    a[0] = -2.0f * c / a0;
    a[1] = (1.0f - alpha) / a0;
}

}  // namespace

float BeatDetector::Biquad::run(float x) {
    // Transposed direct form II.
    const float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
}

BeatDetector::BeatDetector(int sample_rate)
    : sample_rate_(sample_rate), hop_samples_(sample_rate * kHopMs / 1000) {
    float b[3];
    float a[2];
    design(b, a, 40.0f, sample_rate, true);
    high_pass_ = Biquad{b[0], b[1], b[2], a[0], a[1]};
    design(b, a, 150.0f, sample_rate, false);
    low_pass_ = Biquad{b[0], b[1], b[2], a[0], a[1]};
}

void BeatDetector::set_sensitivity(int sensitivity) {
    sensitivity_ = sensitivity < 1 ? 1 : (sensitivity > 10 ? 10 : sensitivity);
}

void BeatDetector::reset() {
    *this = BeatDetector(sample_rate_);
}

uint32_t BeatDetector::time_ms() const {
    return static_cast<uint32_t>(samples_ * 1000u / static_cast<uint64_t>(sample_rate_));
}

void BeatDetector::process(const float* samples, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        const float y = low_pass_.run(high_pass_.run(samples[i]));
        frame_energy_ += y * y;
        ++samples_;
        if (++frame_samples_ == hop_samples_) {
            end_frame();
        }
    }
}

float BeatDetector::onset(int frames_ago) const {
    return onsets_[(next_ - 1 - frames_ago + 2 * kHistory) % kHistory];
}

void BeatDetector::end_frame() {
    const float energy = frame_energy_ / static_cast<float>(hop_samples_);
    frame_energy_ = 0;
    frame_samples_ = 0;

    // Music quieter than the silence level (-60 dBFS at sensitivity 5, 3 dB per step) has no
    // onsets.
    const float silence = 1e-6f * std::pow(2.0f, static_cast<float>(5 - sensitivity_));
    const float log_energy = std::log(energy + 1e-12f);
    float value = 0;
    if (energy > silence) {
        value = log_energy - previous_log_energy_;
        value = value > 0 ? value : 0;
    }
    previous_log_energy_ = log_energy;
    slow_energy_ += (energy - slow_energy_) * 0.01f;

    onsets_[next_] = value;
    next_ = (next_ + 1) % kHistory;
    if (frames_ < kHistory) {
        ++frames_;
    }
    if (++since_estimate_ >= kEstimateEvery && frames_ >= kMaxLag * 3) {
        since_estimate_ = 0;
        estimate();
    }
}

void BeatDetector::estimate() {
    const int n = frames_;
    float mean = 0;
    for (int i = 0; i < n; ++i) {
        mean += onset(i);
    }
    mean /= static_cast<float>(n);

    float zero_lag = 0;
    for (int i = 0; i < n; ++i) {
        const float d = onset(i) - mean;
        zero_lag += d * d;
    }
    if (zero_lag <= 1e-9f) {
        stable_ = false;
        agreeing_estimates_ = 0;
        confidence_ = 0;
        return;
    }

    // Autocorrelation over the tempo range. A small weight prefers tempos near 120 BPM, so a
    // beat at 120 BPM does not read as 60 BPM.
    float best = -1e9f;
    int best_lag = kMinLag;
    float acf[kMaxLag + 2] = {};
    for (int lag = kMinLag - 1; lag <= kMaxLag + 1; ++lag) {
        float sum = 0;
        for (int i = 0; i + lag < n; ++i) {
            sum += (onset(i) - mean) * (onset(i + lag) - mean);
        }
        acf[lag] = sum / zero_lag;
    }
    for (int lag = kMinLag; lag <= kMaxLag; ++lag) {
        const float bpm = 6000.0f / static_cast<float>(lag);
        const float weight = 1.0f - 0.3f * std::fabs(std::log2(bpm / 120.0f));
        const float score = acf[lag] * weight;
        if (score > best && acf[lag] >= acf[lag - 1] && acf[lag] >= acf[lag + 1]) {
            best = score;
            best_lag = lag;
        }
    }
    // Parabolic interpolation of the peak, for a tempo between whole frames.
    const float left = acf[best_lag - 1];
    const float mid = acf[best_lag];
    const float right = acf[best_lag + 1];
    const float denominator = left - 2.0f * mid + right;
    const float shift = denominator != 0 ? 0.5f * (left - right) / denominator : 0.0f;
    const float lag = static_cast<float>(best_lag) + (std::fabs(shift) < 1.0f ? shift : 0.0f);
    const float bpm = 6000.0f / lag;
    confidence_ = mid;

    const bool same = bpm_ > 0 && std::fabs(bpm - bpm_) / bpm_ < 0.04f;
    agreeing_estimates_ = same ? agreeing_estimates_ + 1 : 0;
    bpm_ = bpm;
    const float needed = 0.35f - 0.025f * static_cast<float>(sensitivity_ - 5);
    const bool loud = slow_energy_ > 1e-6f * std::pow(2.0f, static_cast<float>(5 - sensitivity_));
    stable_ = loud && confidence_ >= needed && agreeing_estimates_ >= 3;

    // Beat phase: the offset with the most onset energy at every period back in time.
    const int period = static_cast<int>(lag + 0.5f);
    float best_phase_sum = -1;
    int best_phase = 0;
    for (int p = 0; p < period; ++p) {
        float sum = 0;
        float weight = 1.0f;
        for (int k = p; k < n; k += period) {
            // Allow one frame of timing error on each side.
            const float here = onset(k) + 0.5f * (k > 0 ? onset(k - 1) : 0) +
                               0.5f * (k + 1 < n ? onset(k + 1) : 0);
            sum += here * weight;
            weight *= 0.9f;  // Recent beats count more.
        }
        if (sum > best_phase_sum) {
            best_phase_sum = sum;
            best_phase = p;
        }
    }
    const double period_ms = 60000.0 / bpm;
    const double last_beat_ms = static_cast<double>(time_ms()) - best_phase * kHopMs;
    double next = last_beat_ms + period_ms;
    // Do not repeat a beat that take_beat() already gave.
    while (next < last_beat_given_ms_ + period_ms * 0.5) {
        next += period_ms;
    }
    next_beat_ms_ = next;
}

bool BeatDetector::take_beat(uint32_t at_ms) {
    if (!stable_ || bpm_ <= 0) {
        return false;
    }
    const double period_ms = 60000.0 / bpm_;
    if (static_cast<double>(at_ms) < next_beat_ms_) {
        return false;
    }
    last_beat_given_ms_ = next_beat_ms_;
    next_beat_ms_ += period_ms;
    // After a long pause in the calls, skip the missed beats.
    while (next_beat_ms_ <= static_cast<double>(at_ms)) {
        next_beat_ms_ += period_ms;
    }
    return true;
}

}  // namespace beat
