#pragma once
namespace  lib {

    template <typename T>
    struct bitflag {
        T value;

        constexpr bitflag() : value() {}
        constexpr bitflag(T value) : value(value) {}
        
        template<typename Self>
        constexpr auto operator | (this Self f1, Self f2) {
            return Self{ f1.value | f2.value };
        }

        template<typename Self>
        constexpr auto operator |= (this Self f1, Self f2) {
            return Self{ f1.value | f2.value };
        }

        template<typename Self>
        constexpr auto operator & (this Self f1, Self f2) {
            return Self{ f1.value & f2.value };
        }

        constexpr explicit operator T() {
            return value;
        }

        constexpr bool operator==(this bitflag<T> f1, T f2) {
            return f1.value == f2;
        }
    } ;

}