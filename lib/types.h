#pragma once
#include <stdint.h>
#include <stddef.h>

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

    using intptr  = intptr_t;
    using uintptr = uintptr_t;

    const decltype(nullptr) nil = nullptr;


    struct noncopyable {
        constexpr noncopyable() noexcept {}
        noncopyable(noncopyable const&) = delete;
        noncopyable& operator = (noncopyable const&) = delete;
    } ;

    template<size_t N>
    struct StringLiteral {
        constexpr StringLiteral(const char (&str)[N]) {
            for (size i = 0; i < N; i++) {
                value[i]= str[i];
            }
        }
        
        char value[N];
    } ;

    template<>
    struct StringLiteral<0> {
        constexpr StringLiteral() {}
    } ;

}
