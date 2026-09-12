export module lib.math.bits;
export import lib.types;

import <cmath>;


export extern "C++" {
namespace lib::math {
    inline constexpr float64 NaN = NAN;
    inline constexpr float64 Inf = INFINITY;

    constexpr float64 inf(int sign) {
        if (sign >= 0) {
            return Inf;
        } else {
            return -Inf;
        }
    }

    constexpr float64 nan() {
        return NaN;
    }
}
}
