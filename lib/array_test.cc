import lib.array;
import <initializer_list>;
import <type_traits>;

#include "lib/testing/testing.h"

using IntList = std::initializer_list<int>;
static_assert(std::is_convertible_v<IntList&, lib::view<int>>);
static_assert(std::is_convertible_v<const IntList&, lib::view<int>>);
static_assert(!std::is_constructible_v<lib::view<int>, IntList&&>);
static_assert(!std::is_constructible_v<lib::view<int>, const IntList&&>);
static_assert(!std::is_constructible_v<lib::arr<int>, IntList&>);
static_assert(!std::is_constructible_v<lib::view<double>, IntList&>);
static_assert(std::is_convertible_v<std::initializer_list<int*>&, lib::view<int*>>);
static_assert(!std::is_constructible_v<lib::view<double*>, std::initializer_list<int*>&>);

namespace {
// Forward a named list parameter as a view while its backing array is alive.
int sum_list(std::initializer_list<int> list) {
    auto sum = [](lib::view<int> values) {
        int result = 0;
        for (int value : values) result += value;
        return result;
    };
    return sum(list);
}
}

// Verify borrowing and named-parameter conversion; t reports failures.
void test_initializer_list_view(lib::testing::T &t) {
    const IntList values = {1, 2, 3};
    lib::view<int> borrowed = values;
    const IntList empty = {};
    lib::view<int> empty_view = empty;
    if (borrowed.data != values.begin() || borrowed.len != 3 ||
        borrowed[2] != 3 || empty_view.len != 0 || sum_list({1, 2, 3}) != 6) {
        t.errorf("views must borrow named initializer lists without copying");
    }
}
