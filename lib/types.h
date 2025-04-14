#pragma once
#include <concepts>
#include <stdint.h>
#include <stddef.h>
#include <type_traits>

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
    using intmax = intmax_t;
    using uintmax = uintmax_t;

    const decltype(nullptr) nil = nullptr;

    struct noncopyable {
        noncopyable() = default;
        noncopyable(noncopyable const&) = delete;
        noncopyable& operator = (noncopyable const&) = delete;
    } ;

    struct nonmovable {
        nonmovable() = default;
        nonmovable(nonmovable const&) = delete;
        nonmovable& operator=(nonmovable const&) = delete;
    };

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

    struct numeric {
        template <typename T>
        constexpr auto operator+(this T self, T other) {
            auto [self_val] = self;
            auto [other_val] = other;
            return T { self_val + other_val };
        }

        template <typename T>
        constexpr auto operator-(this T self, T other) {
            auto [self_val] = self;
            auto [other_val] = other;
            return T { self_val - other_val };
        }

        template <typename T>
        constexpr auto operator+=(this T &self, T other) {
            auto &[self_val] = self;
            auto [other_val] = other;

            self_val += other_val;
            return self;
        }

        // T * 2
        template <typename Self, typename Integral>
        requires std::derived_from<Self, numeric> && std::integral<Integral>
        constexpr friend Self operator*(Self self, Integral other) {
            auto [self_val] = self;
            return Self {self_val * other};
        }

        // 2 * T
        template <typename Self, typename Integral>
        requires std::derived_from<Self, numeric> && std::integral<Integral>
        constexpr friend Self operator*(Integral other, Self self) {
            auto [self_val] = self;
            return Self {self_val * other};
        }

        constexpr explicit operator bool(this auto self) {
            auto [self_val] = self;
            return bool(self_val);
        }

        // T <=> T
        template <typename T>
        constexpr auto operator <=> (this T self, T other) {
            auto [self_val] = self;
            auto [other_val] = other;
            return self_val <=> other_val;
        }

        // T <=> int
        template <typename Self, typename Integral>
        requires std::integral<Integral>
        constexpr auto operator <=> (this Self self, Integral other) {
            auto [self_val] = self;
            return self_val <=> other;
        }


        template <typename T>
        using BackingType = decltype([](T *t) { 
            auto [val] = *t;
            return val;
        }(0));

        // conversion operator
        template <typename T>
        constexpr explicit operator BackingType<T>(this T t) {
            auto [val] = t;
            return val;
        }
    } ;

    struct Iterator {
        void operator++() {}
    
        auto operator*(this auto &&t) {
            return t.next();
        }
    
        bool operator!=(this auto &&t, auto&&) {
            return t.has_next();
        }

        auto &begin(this auto &t) {
            return t;
        }

        constexpr bool end() const { return false; }
    } ;
    
    struct Iterable {
        auto begin(this auto &&t) {
            return t.iterator();
        }
    
        constexpr bool end() const { return false; }
    } ;
}
