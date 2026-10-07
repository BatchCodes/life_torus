// SPDX-License-Identifier: GPL-3.0-or-later
#include <cmath>
#include <vector>

#include "spectrum/analyser.hpp"
#include "spectrum/fft.hpp"
#include "tests.hpp"
#include "unity.h"

using spectrum::Analyser;
using spectrum::kBands;

namespace {

constexpr int kRate = 16000;

std::vector<float> sine(float hz, float amplitude, float seconds) {
    std::vector<float> out(static_cast<size_t>(seconds * kRate));
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = amplitude * static_cast<float>(std::sin(2.0 * M_PI * hz * i / kRate));
    }
    return out;
}

int loudest_band(const Analyser& a) {
    int best = 0;
    for (int b = 1; b < kBands; ++b) {
        if (a.bands()[b] > a.bands()[best]) {
            best = b;
        }
    }
    return best;
}

// The band that contains `hz`, from the log spacing of 40 Hz to 8 kHz.
int band_of(float hz) {
    return static_cast<int>(kBands * std::log(hz / spectrum::kLowHz) /
                            std::log(spectrum::kHighHz / spectrum::kLowHz));
}

void test_fft_of_known_signals() {
    constexpr size_t n = 64;
    float re[n] = {};
    float im[n] = {};
    re[0] = 1.0f;  // An impulse: every bin is 1.
    spectrum::fft(re, im, n);
    for (size_t k = 0; k < n; ++k) {
        TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, re[k]);
        TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.0f, im[k]);
    }
    // A cosine of 5 cycles: all energy in bins 5 and n - 5, each n / 2.
    for (size_t i = 0; i < n; ++i) {
        re[i] = static_cast<float>(std::cos(2.0 * M_PI * 5.0 * i / n));
        im[i] = 0.0f;
    }
    spectrum::fft(re, im, n);
    for (size_t k = 0; k < n; ++k) {
        const float magnitude = std::sqrt(re[k] * re[k] + im[k] * im[k]);
        const float expected = (k == 5 || k == n - 5) ? n / 2.0f : 0.0f;
        TEST_ASSERT_FLOAT_WITHIN(1e-3f, expected, magnitude);
    }
}

void test_sine_lights_its_band() {
    for (float hz : {100.0f, 2000.0f}) {
        Analyser a(kRate);
        const std::vector<float> s = sine(hz, 0.3f, 1.0f);
        a.process(s.data(), s.size());
        TEST_ASSERT_TRUE(a.active());
        TEST_ASSERT_INT_WITHIN(1, band_of(hz), loudest_band(a));
        TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, a.bands()[loudest_band(a)]);
        // A band far away stays low.
        const int far = band_of(hz) < kBands / 2 ? kBands - 2 : 1;
        TEST_ASSERT_TRUE(a.bands()[far] < 0.3f);
    }
}

void test_silence_is_inactive() {
    Analyser a(kRate);
    const std::vector<float> loud = sine(500.0f, 0.3f, 0.5f);
    a.process(loud.data(), loud.size());
    TEST_ASSERT_TRUE(a.active());
    const std::vector<float> quiet(kRate * 2, 0.0f);
    a.process(quiet.data(), quiet.size());
    TEST_ASSERT_FALSE(a.active());
    for (float level : a.bands()) {
        TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, level);
    }
}

void test_automatic_gain() {
    // A quiet and a loud tone both reach the full height after the gain settles.
    for (float amplitude : {0.003f, 0.5f}) {
        Analyser a(kRate);
        const std::vector<float> s = sine(1000.0f, amplitude, 2.0f);
        a.process(s.data(), s.size());
        TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.0f, a.bands()[loudest_band(a)]);
    }
}

void test_fft_size_follows_sample_rate() {
    TEST_ASSERT_EQUAL(1024, Analyser(16000).fft_size());
    TEST_ASSERT_EQUAL(2048, Analyser(48000).fft_size());
    Analyser a(16000);
    const std::vector<float> s = sine(300.0f, 0.2f, 0.5f);
    const uint32_t before = a.updates();
    a.process(s.data(), s.size());
    TEST_ASSERT_TRUE(a.updates() > before);
}

}  // namespace

void run_spectrum_tests() {
    RUN_TEST(test_fft_of_known_signals);
    RUN_TEST(test_sine_lights_its_band);
    RUN_TEST(test_silence_is_inactive);
    RUN_TEST(test_automatic_gain);
    RUN_TEST(test_fft_size_follows_sample_rate);
}
