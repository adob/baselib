#pragma once

#include "lib/io/io.h"
#include <type_traits>

namespace lib::strconv {
    namespace detail {
        // format_bits computes the string representation of u in the given base.
        // If neg is set, u is treated as negative int64 value. If append_ is
        // set, the string is appended to dst and the resulting byte slice is
        // returned as the first result value; otherwise the string is returned
        // as the second result value.
        void format_bits(io::Writer &out, uintmax u, int base, bool neg, error err);

        #if __WORDSIZE < 64
        void format_bits(io::Writer &out, uint64 u, int base, bool neg, error err);
        #endif
    }

    template<typename T, typename U>
    using LargerType = std::conditional_t<sizeof(T) >= sizeof(U), T, U>;

    template <typename T>
    struct Formatter : io::WriterTo {
        T val;
        int base;

        Formatter(T t, int base);
        void write_to(io::Writer &out, error) const override;
    } ;

    // format_int returns the string representation of i in the given base,
    // for 2 <= base <= 36. The result uses the lower-case letters 'a' to 'z'
    // for digit values >= 10.
    Formatter<intmax> format_int(int64 i, int base);

    // format_uint returns the string representation of i in the given base,
    // for 2 <= base <= 36. The result uses the lower-case letters 'a' to 'z'
    // for digit values >= 10.
    Formatter<uintmax> format_uint(uint64 i, int base);

    // Itoa is equivalent to [FormatInt](int64(i), 10).
    Formatter<intmax> itoa(int64 i);
}