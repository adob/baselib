#pragma once

#include "lib/base.h"

#include <cmath>
#include <limits>

namespace lib::math {

    constexpr size MaxSize = std::numeric_limits<size>::max();

    auto dist(auto a, auto b) {
        return std::abs(a - b);
    }
    
    constexpr float64 rad2deg(float64 r) { return r * 180.0 / M_PI; }
    constexpr float32 rad2deg(float32 r) { return r * float32(180.0 / M_PI); }
    
    constexpr float64 deg2rad(float64 d) { return d * (M_PI / 180.0); }
    constexpr float32 deg2rad(float32 d) { return d * float32(M_PI / 180.0); }
    
    

    using std::max;
    using std::min;
    
    using std::abs;
    using std::sqrt;
}