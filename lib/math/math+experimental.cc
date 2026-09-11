module;

#include "lib/types.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

export module lib.math;

export import :bits;
export import :matrix;

//import std;

export namespace lib::math {

    inline constexpr size MaxSize = std::numeric_limits<size>::max();

    // Return the absolute difference between a and b.
    auto dist(auto a, auto b) {
        return std::abs(a - b);
    }

    // Convert the angle r from radians to degrees.
    constexpr float64 rad2deg(float64 r) { return r * 180.0 / std::numbers::pi; }
    constexpr float32 rad2deg(float32 r) { return r * float32(180.0 / std::numbers::pi); }

    // Convert the angle d from degrees to radians.
    constexpr float64 deg2rad(float64 d) { return d * (std::numbers::pi / 180.0); }
    constexpr float32 deg2rad(float32 d) { return d * float32(std::numbers::pi / 180.0); }

    using std::max;
    using std::min;

    using std::abs;
    using std::sqrt;
}
