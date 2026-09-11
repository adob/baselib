#pragma once

import lib.array;
import lib.error;
#include <variant>
#include <functional>
#include <tuple>

namespace lib::testing {
    namespace detail {
        struct InternalExample;

        std::tuple<bool, bool>
        run_examples(std::function<bool(str pat, str s, error)> match_string,
                     view<InternalExample> examples);
    }
}
