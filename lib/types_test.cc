#include "lib/fmt/fmt.h"
#include <compare>
#include <cstdint>
#include <stdint.h>
#include <concepts>
#include <type_traits>

import lib.types;

#include "lib/testing/testing.h"

namespace {
struct Count : lib::numeric {
    int value;
    constexpr Count(int value) : value(value) {}
};

static_assert(std::same_as<lib::int32, int32_t>);
static_assert(std::same_as<lib::numeric::BackingType<Count>, int>);
static_assert(lib::nil == nullptr);
static_assert(!std::is_copy_constructible_v<lib::noncopyable>);
static_assert(!std::is_move_constructible_v<lib::nonmovable>);
constexpr lib::StringLiteral literal("hello");
static_assert(literal.value[0] == 'h' && literal.value[5] == '\0');
}

// Check imported numeric operations and backing-value conversion; t reports failures.
void test_types_module_numeric(lib::testing::T &t) {
    Count a(3);
    Count b(4);
    Count sum = a + b;
    a += b;
    if (sum.value != 7 || a.value != 7 || (b * 2).value != 8 ||
        (2 * b).value != 8 || int(b) != 4 || !bool(b) || bool(Count(0)) ||
        !(b < a)) {
        t.errorf("numeric operations must work through import lib.types");
    }
}
