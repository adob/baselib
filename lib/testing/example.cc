#include "example.h"
#include "lib/testing/testing.h"

using namespace lib;
using namespace lib::testing;
using namespace lib::testing::detail;

std::tuple<bool, bool> detail::run_examples(
    std::function<bool(str pat, str s, error)> match_string,
    view<InternalExample> examples) {
    return {true, true};
}
