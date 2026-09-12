module;
#include "example_impl.h"

export module lib.testing.example;
import lib.str;
import lib.array;
import lib.error;
import <variant>;
import <functional>;
import <tuple>;


export extern "C++" {
namespace lib::testing {
    namespace detail {
        struct InternalExample;

        std::tuple<bool, bool>
        run_examples(std::function<bool(str pat, str s, error)> match_string,
                     view<InternalExample> examples);
    }
}

}
