#pragma once

#include "lib/str.h"

namespace lib::strings::detail {
    std::pair<size, size> critical_factorization_fwd(str s);
    std::pair<size, size> critical_factorization_rev(str s);

    size two_way_search_fwd(str haystack, str needle);
    size two_way_search_rev(str haystack, str needle);
}