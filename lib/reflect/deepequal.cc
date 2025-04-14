module;

#include "lib/print.h"

export module lib.reflect:deepequal;

namespace lib::reflect {    
    export template <typename T>
    bool deep_equal(T const *p1, T const *p2) {
        if (typeid(*p1) != typeid(*p2)) {
            return false;
        }

        // auto [...args] = *p1;

        // return deep_equal_i(p1, p2);
        return false;
    }
}