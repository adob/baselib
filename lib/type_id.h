#pragma once

namespace lib {
    using TypeID = void*;

    namespace detail {
        template <typename T>
        inline constexpr char type = 0;
    }
    

    template <typename T>
    inline constexpr TypeID type_id = (TypeID) &detail::type<T>;
}