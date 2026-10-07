// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace beat {

// Finds the tempo and the beat of music from the bass.
//
// The detector filters the bass (approximately 40 Hz to 150 Hz), measures the energy every
// 10 ms, and finds the onsets (the energy rises). The autocorrelation of the onsets gives the
// tempo (60 to 180 BPM). A comb over the recent onsets gives the beat phase, so the detector
// can predict the next beat. The time is the audio time: the number of samples processed.
class BeatDetector {
public:
    explicit BeatDetector(int sample_rate);

    // 1 (only clear, loud beats) to 10 (quiet or weak beats too). Default 5.
    void set_sensitivity(int sensitivity);
    void reset();

    // Processes samples in the range -1 to 1.
    void process(const float* samples, size_t count);

    uint32_t time_ms() const;
    // True when the tempo has been the same for a few seconds and the music is loud enough.
    bool stable() const { return stable_; }
    float bpm() const { return bpm_; }
    float confidence() const { return confidence_; }

    // Returns true once for each predicted beat at or before `at_ms` (audio time), while the
    // beat is stable. Call it often, for example every 10 ms.
    bool take_beat(uint32_t at_ms);

private:
    static constexpr int kHopMs = 10;
    static constexpr int kHistory = 600;       // 6 s of onset values.
    static constexpr int kMinLag = 33;         // 180 BPM.
    static constexpr int kMaxLag = 100;        // 60 BPM.
    static constexpr int kEstimateEvery = 50;  // Frames between tempo estimates (0.5 s).

    struct Biquad {
        float b0, b1, b2, a1, a2;
        float z1 = 0;
        float z2 = 0;
        float run(float x);
    };

    void end_frame();
    void estimate();
    float onset(int frames_ago) const;

    int sample_rate_;
    int hop_samples_;
    int sensitivity_ = 5;
    Biquad high_pass_;
    Biquad low_pass_;

    uint64_t samples_ = 0;
    int frame_samples_ = 0;
    float frame_energy_ = 0;
    float previous_log_energy_ = 0;
    float slow_energy_ = 0;

    std::array<float, kHistory> onsets_{};
    int next_ = 0;
    int frames_ = 0;
    int since_estimate_ = 0;

    float bpm_ = 0;
    float confidence_ = 0;
    int agreeing_estimates_ = 0;
    bool stable_ = false;
    double next_beat_ms_ = 0;
    double last_beat_given_ms_ = -1e9;
};

}  // namespace beat
