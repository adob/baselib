module;


#include <cmath>

export module lib.math:bits;

import lib.types;

export namespace lib::math {
    constexpr float64 NaN = NAN;
    constexpr float64 Inf = INFINITY;

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