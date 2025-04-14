#pragma once

#include "lib/error.h"
#include "lib/str.h"
#include "lib/testing/testing.h"
#include <functional>
#include <tuple>

namespace lib::testing {
    namespace detail {
        struct InternalTest;

        std::tuple<bool, bool>
        run_examples(std::function<bool(str pat, str s, error)> match_string,
                     view<InternalExample> examples);
    }
}