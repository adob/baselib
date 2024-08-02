#pragma once
#include <stdint.h>
#include <stddef.h>

//#include "pw_span/span.h"

namespace lib {

    using uint    = unsigned int;
    using ushort  = unsigned short;
    using ulong   = unsigned long;
    using ssize   = intptr_t;
    using usize   = size_t;
    using size    = ssize;
    using byte    = uint8_t;
    using rune    = char32_t;
    using char16  = char16_t;
    using char32  = char32_t;
    using int8    = int8_t;
    using int16   = int16_t;
    using int32   = int32_t;
    using int64   = int64_t;
    //using int128  = __int128;
    using uint8   = uint8_t;
    using uint16  = uint16_t;
    using uint32  = uint32_t;
    using uint64  = uint64_t;

    using float32 = float;
    using float64 = double;

    const decltype(nullptr) nil = nullptr;

    //using bytes = pw::span<const byte>;

    struct noncopyable {
        constexpr noncopyable() noexcept {}
        noncopyable(noncopyable const&) = delete;
        noncopyable& operator = (noncopyable const&) = delete;
    } ;

}
