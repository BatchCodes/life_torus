// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstring>
#include <unordered_map>

#include "life/grid.hpp"
#include "patterns/library.hpp"
#include "patterns/presets.hpp"
#include "patterns/shape.hpp"
#include "tests.hpp"
#include "unity.h"

using life::EdgeMode;
using life::Grid;
using patterns::Shape;

namespace {

Shape shape_named(const char* name) {
    const int index = patterns::find_shape(name);
    TEST_ASSERT_TRUE_MESSAGE(index >= 0, name);
    return patterns::load_shape(index);
}

Grid placed(const Shape& shape, int centre_x, int centre_y) {
    Grid grid;
    patterns::stamp(grid, shape, centre_x, centre_y, EdgeMode::kTorus);
    return grid;
}

Grid shifted(const Grid& grid, int dx, int dy) {
    Grid result;
    for (int y = 0; y < life::kHeight; ++y) {
        for (int x = 0; x < life::width(); ++x) {
            if (grid.get(x, y)) {
                result.set(life::wrap_x(x + dx), life::wrap_y(y + dy), true);
            }
        }
    }
    return result;
}

Grid run(Grid grid, int generations) {
    for (int i = 0; i < generations; ++i) {
        grid = life::step(grid, EdgeMode::kTorus);
    }
    return grid;
}

// True if the grid comes back after `generations`, moved by a distance of `distance` cells in
// one of the 8 directions.
bool moves_by(const Grid& grid, int generations, int distance) {
    const Grid later = run(grid, generations);
    for (int dy = -distance; dy <= distance; dy += distance) {
        for (int dx = -distance; dx <= distance; dx += distance) {
            if ((dx != 0 || dy != 0) && later == shifted(grid, dx, dy)) {
                return true;
            }
        }
    }
    return false;
}

void test_rle_parse() {
    Shape shape;
    TEST_ASSERT_TRUE(patterns::parse_rle("bo$2bo$3o!", shape));
    TEST_ASSERT_EQUAL(3, shape.width());
    TEST_ASSERT_EQUAL(3, shape.height());
    TEST_ASSERT_EQUAL(5, shape.population());
    TEST_ASSERT_TRUE(shape.get(1, 0));
    TEST_ASSERT_TRUE(shape.get(2, 1));
    TEST_ASSERT_TRUE(shape.get(0, 2));

    TEST_ASSERT_TRUE(
        patterns::parse_rle("#N Glider\nx = 3, y = 3, rule = B3/S23\nbo$2bo$3o!", shape));
    TEST_ASSERT_EQUAL(5, shape.population());

    TEST_ASSERT_TRUE(patterns::parse_rle("o2$o!", shape));
    TEST_ASSERT_EQUAL(3, shape.height());

    TEST_ASSERT_FALSE(patterns::parse_rle("bo$2bo$3o", shape));  // No '!'.
    TEST_ASSERT_FALSE(patterns::parse_rle("65o!", shape));       // Wider than the grid.
    TEST_ASSERT_FALSE(patterns::parse_rle("o32$o!", shape));     // Higher than the grid.
    TEST_ASSERT_FALSE(patterns::parse_rle("bz!", shape));
}

void test_library_shapes_parse() {
    struct Expected {
        const char* name;
        int population;
    };
    constexpr Expected kExpected[] = {
        {"Cell", 1},
        {"Glider", 5},
        {"Lightweight spaceship", 9},
        {"Middleweight spaceship", 11},
        {"Heavyweight spaceship", 13},
        {"Pulsar", 48},
        {"Pentadecathlon", 12},
        {"R-pentomino", 5},
        {"Acorn", 7},
        {"Diehard", 7},
        {"Pi-heptomino", 7},
        {"Gosper glider gun", 36},
        {"Simkin glider gun", 36},
    };
    for (const auto& info : patterns::cursor_shapes()) {
        Shape shape;
        TEST_ASSERT_TRUE_MESSAGE(patterns::parse_rle(info.rle, shape), info.name);
        TEST_ASSERT_TRUE_MESSAGE(shape.population() > 0, info.name);
    }
    for (const auto& expected : kExpected) {
        TEST_ASSERT_EQUAL_MESSAGE(expected.population, shape_named(expected.name).population(),
                                  expected.name);
    }
    TEST_ASSERT_EQUAL(0, std::strcmp(patterns::cursor_shapes()[0].name, "Cell"));
}

void test_still_lifes_are_still() {
    for (const auto& info : patterns::cursor_shapes()) {
        if (info.group != patterns::Group::kStillLife) {
            continue;
        }
        const Grid grid = placed(patterns::load_shape(patterns::find_shape(info.name)), 20, 15);
        TEST_ASSERT_TRUE_MESSAGE(life::step(grid, EdgeMode::kTorus) == grid, info.name);
    }
}

void test_oscillator_periods() {
    struct Expected {
        const char* name;
        int period;
    };
    constexpr Expected kExpected[] = {
        {"Blinker", 2}, {"Toad", 2}, {"Beacon", 2}, {"Pulsar", 3}, {"Pentadecathlon", 15},
    };
    for (const auto& expected : kExpected) {
        const Grid grid = placed(shape_named(expected.name), 32, 16);
        for (int p = 1; p < expected.period; ++p) {
            TEST_ASSERT_TRUE_MESSAGE(run(grid, p) != grid, expected.name);
        }
        TEST_ASSERT_TRUE_MESSAGE(run(grid, expected.period) == grid, expected.name);
    }
}

void test_spaceships_move() {
    TEST_ASSERT_TRUE(moves_by(placed(shape_named("Glider"), 20, 10), 4, 1));
    TEST_ASSERT_TRUE(moves_by(placed(shape_named("Lightweight spaceship"), 20, 10), 4, 2));
    TEST_ASSERT_TRUE(moves_by(placed(shape_named("Middleweight spaceship"), 20, 10), 4, 2));
    TEST_ASSERT_TRUE(moves_by(placed(shape_named("Heavyweight spaceship"), 20, 10), 4, 2));
}

void test_glider_survives_each_turn_and_mirror() {
    Shape shape = shape_named("Glider");
    for (int turn = 0; turn < 4; ++turn) {
        TEST_ASSERT_TRUE(moves_by(placed(shape, 30, 15), 4, 1));
        TEST_ASSERT_TRUE(moves_by(placed(patterns::mirror(shape), 30, 15), 4, 1));
        shape = patterns::rotate(shape);
    }
}

void test_rotate_and_mirror() {
    const Shape lwss = shape_named("Lightweight spaceship");
    const Shape turned = patterns::rotate(lwss);
    TEST_ASSERT_EQUAL(lwss.height(), turned.width());
    TEST_ASSERT_EQUAL(lwss.width(), turned.height());
    TEST_ASSERT_EQUAL(lwss.population(), turned.population());
    // Four turns give the original shape.
    const Shape back = patterns::rotate(patterns::rotate(patterns::rotate(turned)));
    TEST_ASSERT_TRUE(placed(back, 10, 10) == placed(lwss, 10, 10));
    // Two mirrors give the original shape.
    TEST_ASSERT_TRUE(placed(patterns::mirror(patterns::mirror(lwss)), 10, 10) ==
                     placed(lwss, 10, 10));

    // The Gosper gun is wider than the grid is high, so a turn is 180 degrees.
    const Shape gun = shape_named("Gosper glider gun");
    const Shape gun_turned = patterns::rotate(gun);
    TEST_ASSERT_EQUAL(gun.width(), gun_turned.width());
    TEST_ASSERT_EQUAL(gun.height(), gun_turned.height());
    TEST_ASSERT_TRUE(placed(patterns::rotate(gun_turned), 32, 16) == placed(gun, 32, 16));
}

void test_stamp_wraps_across_seam() {
    const Shape blinker = shape_named("Blinker");
    Grid grid;
    patterns::stamp(grid, blinker, 0, 5, EdgeMode::kCylinder);
    TEST_ASSERT_TRUE(grid.get(63, 5));
    TEST_ASSERT_TRUE(grid.get(0, 5));
    TEST_ASSERT_TRUE(grid.get(1, 5));
    TEST_ASSERT_EQUAL(3, grid.population());

    patterns::erase(grid, shape_named("Cell"), 0, 5, EdgeMode::kCylinder);
    TEST_ASSERT_EQUAL(2, grid.population());
}

void test_stamp_top_edge() {
    const Shape vertical = patterns::rotate(shape_named("Blinker"));
    Grid torus;
    patterns::stamp(torus, vertical, 10, 0, EdgeMode::kTorus);
    TEST_ASSERT_TRUE(torus.get(10, 31));
    TEST_ASSERT_EQUAL(3, torus.population());

    Grid cylinder;
    patterns::stamp(cylinder, vertical, 10, 0, EdgeMode::kCylinder);
    TEST_ASSERT_FALSE(cylinder.get(10, 31));
    TEST_ASSERT_EQUAL(2, cylinder.population());
}

void test_guns_fire_gliders() {
    // The Gosper gun has period 30 and the Simkin gun has period 120. Each period adds one
    // glider of 5 cells.
    const Grid gosper = placed(shape_named("Gosper glider gun"), 32, 8);
    TEST_ASSERT_EQUAL(36, gosper.population());
    TEST_ASSERT_EQUAL(41, run(gosper, 30).population());
    TEST_ASSERT_EQUAL(46, run(gosper, 60).population());

    const Grid simkin = placed(shape_named("Simkin glider gun"), 20, 14);
    TEST_ASSERT_EQUAL(36, simkin.population());
    TEST_ASSERT_EQUAL(41, run(simkin, 120).population());
}

// The same measurement as tools/preset_survey. docs/software/presets.md has the full table.
void test_preset_lifetimes() {
    struct Expected {
        const char* name;
        int settles_at;
        int period;  // 0: the board becomes empty.
    };
    constexpr Expected kExpected[] = {
        {"Glider gun", 662, 2},   {"R-pentomino", 1199, 2},     {"Spaceship fleet", 159, 2},
        {"Acorn", 340, 2},        {"Oscillator garden", 0, 30}, {"Glider swarm", 293, 2},
        {"Diehard", 130, 0},      {"Pi-heptomino", 173, 2},     {"Simkin gun", 469, 6},
        {"Random soup", 3313, 2},
    };
    const auto all = patterns::presets();
    TEST_ASSERT_EQUAL(static_cast<int>(std::size(kExpected)) + 1, static_cast<int>(all.size()));
    for (const auto& expected : kExpected) {
        const patterns::Preset* preset = nullptr;
        for (const auto& candidate : all) {
            if (std::strcmp(candidate.name, expected.name) == 0) {
                preset = &candidate;
            }
        }
        TEST_ASSERT_NOT_NULL_MESSAGE(preset, expected.name);
        Grid grid;
        life::Rng rng(1);
        const EdgeMode mode = patterns::build_preset(*preset, grid, rng, EdgeMode::kTorus, 30);
        std::unordered_map<uint32_t, int> seen;
        int settles_at = -1;
        int period = -1;
        for (int generation = 0; generation < 20000 && settles_at < 0; ++generation) {
            if (grid.empty()) {
                settles_at = generation;
                period = 0;
                break;
            }
            const auto [it, inserted] = seen.emplace(grid.hash(), generation);
            if (!inserted) {
                settles_at = it->second;
                period = generation - it->second;
                break;
            }
            grid = life::step(grid, mode);
        }
        TEST_ASSERT_EQUAL_MESSAGE(expected.settles_at, settles_at, expected.name);
        TEST_ASSERT_EQUAL_MESSAGE(expected.period, period, expected.name);
    }
}

void test_empty_preset() {
    const auto all = patterns::presets();
    const patterns::Preset& empty = all[all.size() - 1];
    TEST_ASSERT_EQUAL(static_cast<int>(patterns::PresetKind::kEmpty), static_cast<int>(empty.kind));
    Grid grid;
    grid.set(1, 1, true);
    life::Rng rng(1);
    TEST_ASSERT_EQUAL(
        static_cast<int>(EdgeMode::kCylinder),
        static_cast<int>(patterns::build_preset(empty, grid, rng, EdgeMode::kCylinder, 30)));
    TEST_ASSERT_TRUE(grid.empty());
}

}  // namespace

void run_patterns_tests() {
    RUN_TEST(test_rle_parse);
    RUN_TEST(test_library_shapes_parse);
    RUN_TEST(test_still_lifes_are_still);
    RUN_TEST(test_oscillator_periods);
    RUN_TEST(test_spaceships_move);
    RUN_TEST(test_glider_survives_each_turn_and_mirror);
    RUN_TEST(test_rotate_and_mirror);
    RUN_TEST(test_stamp_wraps_across_seam);
    RUN_TEST(test_stamp_top_edge);
    RUN_TEST(test_guns_fire_gliders);
    RUN_TEST(test_preset_lifetimes);
    RUN_TEST(test_empty_preset);
}
