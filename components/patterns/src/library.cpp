// SPDX-License-Identifier: GPL-3.0-or-later
//
// Pattern data in RLE format from the LifeWiki (https://conwaylife.com/wiki/).
#include "patterns/library.hpp"

#include <cstring>

namespace patterns {

namespace {

constexpr ShapeInfo kShapes[] = {
    {"Cell", Group::kSingle, "o!"},

    {"Block", Group::kStillLife, "2o$2o!"},
    {"Beehive", Group::kStillLife, "b2o$o2bo$b2o!"},
    {"Loaf", Group::kStillLife, "b2o$o2bo$bobo$2bo!"},
    {"Boat", Group::kStillLife, "2o$obo$bo!"},
    {"Tub", Group::kStillLife, "bo$obo$bo!"},

    {"Blinker", Group::kOscillator, "3o!"},
    {"Toad", Group::kOscillator, "b3o$3o!"},
    {"Beacon", Group::kOscillator, "2o$2o$2b2o$2b2o!"},
    {"Pulsar", Group::kOscillator,
     "2b3o3b3o2b2$o4bobo4bo$o4bobo4bo$o4bobo4bo$2b3o3b3o2b2$2b3o3b3o2b$o4bobo4bo$"
     "o4bobo4bo$o4bobo4bo2$2b3o3b3o!"},
    {"Pentadecathlon", Group::kOscillator, "2bo4bo2b$2ob4ob2o$2bo4bo!"},

    {"Glider", Group::kSpaceship, "bo$2bo$3o!"},
    {"Lightweight spaceship", Group::kSpaceship, "bo2bo$o4b$o3bo$4o!"},
    {"Middleweight spaceship", Group::kSpaceship, "3bo2b$bo3bo$o5b$o4bo$5o!"},
    {"Heavyweight spaceship", Group::kSpaceship, "3b2o2b$bo4bo$o6b$o5bo$6o!"},

    {"R-pentomino", Group::kMethuselah, "b2o$2o$bo!"},
    {"Acorn", Group::kMethuselah, "bo5b$3bo3b$2o2b3o!"},
    {"Diehard", Group::kMethuselah, "6bob$2o6b$bo3b3o!"},
    {"Pi-heptomino", Group::kMethuselah, "3o$obo$obo!"},

    {"Gosper glider gun", Group::kGun,
     "24bo$22bobo$12b2o6b2o12b2o$11bo3bo4b2o12b2o$2o8bo5bo3b2o$2o8bo3bob2o4bobo$"
     "10bo5bo7bo$11bo3bo$12b2o!"},
    {"Simkin glider gun", Group::kGun,
     "2o5b2o$2o5b2o2$4b2o$4b2o5$22b2ob2o$21bo5bo$21bo6bo2b2o$21b3o3bo3b2o$26bo4$"
     "20b2o$20bo$21b3o$23bo!"},
};

}  // namespace

std::span<const ShapeInfo> cursor_shapes() {
    return kShapes;
}

Shape load_shape(int index) {
    Shape shape;
    parse_rle(kShapes[index].rle, shape);
    return shape;
}

bool shape_fits(int index) {
    return load_shape(index).width() <= life::width();
}

int find_shape(const char* name) {
    for (int i = 0; i < static_cast<int>(std::size(kShapes)); ++i) {
        if (std::strcmp(kShapes[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

const char* group_name(Group group) {
    switch (group) {
        case Group::kSingle:
            return "Single cell";
        case Group::kStillLife:
            return "Still life";
        case Group::kOscillator:
            return "Oscillator";
        case Group::kSpaceship:
            return "Spaceship";
        case Group::kMethuselah:
            return "Methuselah";
        case Group::kGun:
            return "Gun";
    }
    return "";
}

}  // namespace patterns
