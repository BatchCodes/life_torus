// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <span>

#include "patterns/shape.hpp"

namespace patterns {

enum class Group : uint8_t { kSingle, kStillLife, kOscillator, kSpaceship, kMethuselah, kGun };

struct ShapeInfo {
    const char* name;
    Group group;
    const char* rle;
};

// The cursor shapes, in the order that L and R step through them. The first is a single cell.
std::span<const ShapeInfo> cursor_shapes();

// Parses one entry of cursor_shapes(). The library is fixed, so this cannot fail.
Shape load_shape(int index);

// The index of the shape with this name in cursor_shapes(), or -1.
int find_shape(const char* name);

const char* group_name(Group group);

}  // namespace patterns
