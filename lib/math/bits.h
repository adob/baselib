#pragma  once
import lib.types;

#include <cmath>

namespace lib::math {
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