export module lib.utils;
import <algorithm>;

// Preserve compatibility with declarations in the remaining headers.
export extern "C++" {
namespace lib {
    template <typename F>
    struct defer {
        F f;
        constexpr defer(F &&f) : f(f) {}

        ~defer() {
            f();
        }
    } ;

    using std::min;
    using std::max;
}
}
