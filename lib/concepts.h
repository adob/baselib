#pragma once

#include <concepts>

namespace lib::concepts {
    template <typename T>
    concept SimpleVector = requires(T t){
        { t.size() } -> std::convertible_to<std::size_t>;
        requires std::is_pointer_v<decltype(t.data())>;
    } ;

    template <typename T>
    concept Sizeable = requires(T const &t){
        { t.size() } -> std::convertible_to<std::size_t>;
    } ;
}