// SPDX-License-Identifier: GPL-3.0-or-later
#include <cmath>
#include <vector>

#include "beat/beat_detector.hpp"
#include "life/grid.hpp"
#include "tests.hpp"
#include "unity.h"

using beat::BeatDetector;

namespace {

constexpr int kRate = 16000;

// Generated music: a bass drum (55 Hz, short decay) on each beat, and optionally a louder
// hi-hat (high noise) half-way between the beats.
struct Song {
    float bpm;
    float kick = 0.5f;
    float hat = 0.0f;
    float noise = 0.001f;
    float start_s = 0;  // Beat phase: the first kick.
};

class Generator {
public:
    explicit Generator(uint32_t seed) : rng_(seed) {}

    // Appends `seconds` of the song, and the times of its kicks (ms).
    void add(const Song& song, float seconds, std::vector<float>& out, std::vector<double>& kicks) {
        const double period = 60.0 / song.bpm;
        const size_t count = static_cast<size_t>(seconds * kRate);
        for (size_t i = 0; i < count; ++i, ++n_) {
            const double t = static_cast<double>(n_) / kRate;
            double since = std::fmod(t - song.start_s + 1000 * period, period);
            if (since < 1.0 / kRate) {
                kicks.push_back(t * 1000.0);
            }
            float x = 0;
            x += song.kick *
                 static_cast<float>(std::exp(-since / 0.08) * std::sin(2 * M_PI * 55.0 * since));
            const double half = std::fmod(since + period / 2, period);
            const float white = static_cast<float>(rng_.below(20001)) / 10000.0f - 1.0f;
            const float hat_noise = white - previous_white_;  // A difference removes the bass.
            previous_white_ = white;
            x += song.hat * static_cast<float>(std::exp(-half / 0.03)) * hat_noise;
            x += song.noise * white;
            out.push_back(x);
        }
    }

private:
    life::Rng rng_;
    uint64_t n_ = 0;
    float previous_white_ = 0;
};

// Runs the detector in 10 ms blocks. Records the beats it gives after `listen_from_ms`.
std::vector<double> run(BeatDetector& d, const std::vector<float>& audio, double listen_from_ms) {
    std::vector<double> beats;
    const size_t block = kRate / 100;
    for (size_t i = 0; i + block <= audio.size(); i += block) {
        d.process(audio.data() + i, block);
        if (d.take_beat(d.time_ms()) && d.time_ms() >= listen_from_ms) {
            beats.push_back(d.time_ms());
        }
    }
    return beats;
}

// The largest distance from a given beat to the nearest kick.
double worst_offset(const std::vector<double>& beats, const std::vector<double>& kicks) {
    double worst = 0;
    for (double b : beats) {
        double nearest = 1e9;
        for (double k : kicks) {
            nearest = std::fmin(nearest, std::fabs(b - k));
        }
        worst = std::fmax(worst, nearest);
    }
    return worst;
}

void check_tempo(float bpm, float hat) {
    BeatDetector d(kRate);
    Generator g(3);
    std::vector<float> audio;
    std::vector<double> kicks;
    Song song{bpm};
    song.hat = hat;
    song.start_s = 0.137f;
    g.add(song, 14.0f, audio, kicks);
    const std::vector<double> beats = run(d, audio, 10000);
    TEST_ASSERT_TRUE(d.stable());
    TEST_ASSERT_FLOAT_WITHIN(bpm * 0.02f, bpm, d.bpm());
    // 4 s of beats, each within 40 ms of a kick.
    TEST_ASSERT_INT_WITHIN(1, static_cast<int>(4.0f * bpm / 60.0f), static_cast<int>(beats.size()));
    TEST_ASSERT_TRUE(worst_offset(beats, kicks) < 40.0);
}

void test_120_bpm() {
    check_tempo(120.0f, 0.0f);
}

void test_90_bpm() {
    check_tempo(90.0f, 0.0f);
}

void test_follows_bass_not_hi_hat() {
    // The hi-hat is louder than the kick, but between the beats. The beats must stay on the kicks.
    check_tempo(110.0f, 1.0f);
}

void test_tempo_change() {
    BeatDetector d(kRate);
    Generator g(5);
    std::vector<float> audio;
    std::vector<double> kicks;
    g.add(Song{120.0f}, 10.0f, audio, kicks);
    g.add(Song{90.0f}, 12.0f, audio, kicks);
    const std::vector<double> beats = run(d, audio, 19000);
    TEST_ASSERT_TRUE(d.stable());
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 90.0f, d.bpm());
    TEST_ASSERT_TRUE(worst_offset(beats, kicks) < 40.0);
}

void test_silence_has_no_beat() {
    BeatDetector d(kRate);
    std::vector<float> audio(kRate * 10, 0.0f);
    const std::vector<double> beats = run(d, audio, 0);
    TEST_ASSERT_FALSE(d.stable());
    TEST_ASSERT_EQUAL(0, static_cast<int>(beats.size()));
}

void test_noise_has_no_beat() {
    BeatDetector d(kRate);
    Generator g(9);
    std::vector<float> audio;
    std::vector<double> kicks;
    Song song{120.0f};
    song.kick = 0;
    song.noise = 0.3f;
    g.add(song, 10.0f, audio, kicks);
    const std::vector<double> beats = run(d, audio, 0);
    TEST_ASSERT_FALSE(d.stable());
    TEST_ASSERT_EQUAL(0, static_cast<int>(beats.size()));
}

void test_sensitivity_for_quiet_music() {
    Song quiet{120.0f};
    quiet.kick = 0.004f;
    quiet.noise = 0.00005f;
    for (int sensitivity : {1, 10}) {
        BeatDetector d(kRate);
        d.set_sensitivity(sensitivity);
        Generator g(11);
        std::vector<float> audio;
        std::vector<double> kicks;
        g.add(quiet, 12.0f, audio, kicks);
        run(d, audio, 0);
        TEST_ASSERT_EQUAL_MESSAGE(sensitivity == 10, d.stable(), sensitivity == 10 ? "10" : "1");
    }
}

}  // namespace

void run_beat_tests() {
    RUN_TEST(test_120_bpm);
    RUN_TEST(test_90_bpm);
    RUN_TEST(test_follows_bass_not_hi_hat);
    RUN_TEST(test_tempo_change);
    RUN_TEST(test_silence_has_no_beat);
    RUN_TEST(test_noise_has_no_beat);
    RUN_TEST(test_sensitivity_for_quiet_music);
}
