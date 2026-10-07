// SPDX-License-Identifier: GPL-3.0-or-later
#include "colour/colour.hpp"
#include "tests.hpp"
#include "unity.h"

using colour::ColourConfig;
using colour::Rgb;
using colour::RgbImage;
using colour::Scheme;
using colour::Source;
using frame::Level;

namespace {

ColourConfig full() {
    ColourConfig config;
    config.brightness_percent = 100;
    config.current_limit_ma = 1000000;
    return config;
}

void test_hsv() {
    TEST_ASSERT_TRUE(colour::hsv(0, 255) == (Rgb{255, 0, 0}));
    TEST_ASSERT_TRUE(colour::hsv(120, 255) == (Rgb{0, 255, 0}));
    TEST_ASSERT_TRUE(colour::hsv(240, 255) == (Rgb{0, 0, 255}));
    TEST_ASSERT_TRUE(colour::hsv(360, 255) == (Rgb{255, 0, 0}));
}

void test_life_state_colours() {
    frame::Image image;
    image.set(0, 0, Level::kBright);
    image.set(1, 0, Level::kNormal);
    image.set(2, 0, Level::kDim);
    image.set(3, 0, Level::kHighlight);
    RgbImage out;
    colour::colourise(image, Source::kGameOfLife, 0, full(), out);
    TEST_ASSERT_TRUE(out.get(0, 0).g > out.get(0, 0).r);  // Born: green.
    TEST_ASSERT_TRUE(out.get(1, 0).b > out.get(1, 0).g);  // Survives: blue.
    TEST_ASSERT_TRUE(out.get(2, 0).r > out.get(2, 0).b);  // Dies next: red.
    TEST_ASSERT_TRUE(out.get(3, 0) == (Rgb{255, 255, 255}));
    TEST_ASSERT_TRUE(out.get(4, 0) == (Rgb{}));
}

void test_single_colour_follows_levels() {
    frame::Image image;
    image.set(0, 0, Level::kBright);
    image.set(1, 0, Level::kDim);
    ColourConfig config = full();
    config.scheme = Scheme::kSingle;
    config.single = Rgb{0, 200, 100};
    RgbImage out;
    colour::colourise(image, Source::kRain, 0, config, out);
    TEST_ASSERT_TRUE(out.get(0, 0) == (Rgb{0, 200, 100}));
    TEST_ASSERT_EQUAL(0, out.get(1, 0).r);
    TEST_ASSERT_TRUE(out.get(1, 0).g < 60);
}

void test_rainbow_and_visualiser_hues() {
    frame::Image image;
    for (int x = 0; x < life::width(); ++x) {
        image.set(x, 31, Level::kBright);
    }
    RgbImage out;
    colour::colourise(image, Source::kVisualiser, 0, full(), out);
    const Rgb bass = out.get(0, 31);
    const Rgb treble = out.get(life::width() - 1, 31);
    TEST_ASSERT_TRUE(bass.r > 200 && bass.b < 50);      // Red.
    TEST_ASSERT_TRUE(treble.b > 200 && treble.g < 50);  // Violet.
    colour::colourise(image, Source::kRain, 0, full(), out);
    TEST_ASSERT_FALSE(out.get(0, 31) == out.get(life::width() / 3, 31));
}

void test_brightness_scales() {
    frame::Image image;
    image.set(0, 0, Level::kHighlight);
    ColourConfig config = full();
    config.brightness_percent = 25;
    RgbImage out;
    colour::colourise(image, Source::kGameOfLife, 0, config, out);
    TEST_ASSERT_UINT8_WITHIN(1, 64, out.get(0, 0).r);
}

void test_current_limit() {
    // All 2048 LEDs white at full brightness: far over any USB-C supply.
    frame::Image image;
    image.fill(Level::kHighlight);
    ColourConfig config = full();
    RgbImage out;
    colour::colourise(image, Source::kGameOfLife, 0, config, out);
    const uint32_t before = colour::estimate_ma(out, config);
    TEST_ASSERT_TRUE(before > 100000);  // About 2048 × 60 mA.
    config.current_limit_ma = 4500;
    const int amount = colour::limit_current(out, config);
    TEST_ASSERT_TRUE(amount < 256);
    TEST_ASSERT_TRUE(colour::estimate_ma(out, config) <= 4500);
    // The idle current alone: 2048 × 0.8 mA.
    frame::Image dark;
    colour::colourise(dark, Source::kGameOfLife, 0, config, out);
    TEST_ASSERT_UINT32_WITHIN(5, 1638, colour::estimate_ma(out, config));
    TEST_ASSERT_EQUAL(256, colour::limit_current(out, config));
}

}  // namespace

void run_colour_tests() {
    RUN_TEST(test_hsv);
    RUN_TEST(test_life_state_colours);
    RUN_TEST(test_single_colour_follows_levels);
    RUN_TEST(test_rainbow_and_visualiser_hues);
    RUN_TEST(test_brightness_scales);
    RUN_TEST(test_current_limit);
}
