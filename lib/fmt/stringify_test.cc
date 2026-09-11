#include "fmt.h"
#include "lib/testing/testing.h"

#include <type_traits>

using namespace lib;

namespace {
// Check both operand orders, including C++20 rewritten comparison candidates.
template <typename L, typename R>
concept EqualityComparable = requires(const L &left, const R &right) {
    left == right;
    right == left;
    left != right;
    right != left;
};

template <typename L, typename R>
concept HasEquality = requires(const L &left, const R &right) { left == right; };

template <typename L, typename R>
concept HasInequality = requires(const L &left, const R &right) { left != right; };

template <typename L, typename R>
concept Ordered = requires(const L &left, const R &right) { left < right; };

// Every comparison must be rejected individually, in either operand order.
template <typename L, typename R>
concept NoComparisons = !HasEquality<L, R> && !HasEquality<R, L>
    && !HasInequality<L, R> && !HasInequality<R, L>
    && !Ordered<L, R> && !Ordered<R, L>;

using Formatted = fmt::Stringifier<int>;
static_assert(std::is_convertible_v<Formatted, String>);
static_assert(std::is_convertible_v<const Formatted &, String>);
static_assert(!std::is_convertible_v<Formatted, str>);
static_assert(NoComparisons<str, Formatted>);
static_assert(NoComparisons<String, Formatted>);
static_assert(EqualityComparable<String, String>);
static_assert(EqualityComparable<String, str>);
static_assert(EqualityComparable<String, char[3]>);
static_assert(Ordered<String, String>);
}

// Verify implicit ownership and explicit comparison conversion using test reporter t.
void test_stringify_conversions(testing::T &t) {
    String text = fmt::stringify(42);
    if (text != "42" || text != String(fmt::stringify(42))) {
        t.errorf("stringify should produce the owning string 42");
    }
    text = fmt::stringify(7);
    str view = text;
    if (view != "7" || view != String(fmt::stringify(7))) {
        t.errorf("stringify assignment and explicit comparison should produce 7");
    }
    if (!(String("a") < String("b"))) {
        t.errorf("owning string ordering should remain lexicographic");
    }
    const int value = 9;
    const auto formatted = fmt::stringify(value);
    String constant = formatted;
    if (constant != "9") {
        t.errorf("a const stringifier should also convert to an owning string");
    }
}
