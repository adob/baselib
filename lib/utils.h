#pragma once

#include <algorithm>
namespace lib {
    template <typename F>
    struct defer {
        F f;
        constexpr defer(F &&f) : f(f) {}

        ~defer() {
            f();
        }
    } ;

    using std::min;
    using std::max;
}