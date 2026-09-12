module;
#include "two_way_impl.h"

export module lib.strings.two_way;
export import lib.types;
import <utility>;
export import lib.str;

export extern "C++" {
namespace lib::strings::detail {
    std::pair<size, size> critical_factorization_fwd(str s);
    std::pair<size, size> critical_factorization_rev(str s);

    size two_way_search_fwd(str haystack, str needle);
    size two_way_search_rev(str haystack, str needle);
}
}
