// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace beat {

// Where the beat comes from.
enum class Source : uint8_t { kNone, kBass, kFullRange };

// Finds the tempo and the beat of music.
//
// Two channels listen side by side: the bass (approximately 40 Hz to 150 Hz) and the full range
// (approximately 150 Hz to 6 kHz). Each channel measures its energy every 10 ms, finds the
// onsets (the energy rises), the tempo (60 to 180 BPM, from the autocorrelation of the onsets)
// and the beat phase (from a comb over the recent onsets). The detector uses the bass beat when
// it is stable, and the full-range beat otherwise. This also finds a beat in music from a phone
// speaker, which plays almost no bass. The time is the audio time: the samples processed.
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
    bool stable() const { return source() != Source::kNone; }
    Source source() const;
    float bpm() const;
    // The current tempo estimate, also before it is stable, or 0 when there is no useful
    // estimate yet (for example in silence).
    float bpm_estimate() const;

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

    // One frequency band with its own onsets, tempo and phase.
    struct Channel {
        Biquad high_pass;
        Biquad low_pass;
        float frame_energy = 0;
        float previous_log_energy = 0;
        float slow_energy = 0;
        std::array<float, kHistory> onsets{};
        float bpm = 0;
        float confidence = 0;
        int agreeing = 0;
        bool stable = false;
        double next_beat_ms = 0;

        float onset(int next, int frames_ago) const;
    };

    void end_frame();
    void end_frame(Channel& c, float silence);
    void estimate(Channel& c);
    const Channel* active() const;

    int sample_rate_;
    int hop_samples_;
    int sensitivity_ = 5;
    Channel bass_;
    Channel full_;

    uint64_t samples_ = 0;
    int frame_samples_ = 0;
    int next_ = 0;
    int frames_ = 0;
    int since_estimate_ = 0;
    double last_beat_given_ms_ = -1e9;
};

}  // namespace beat
