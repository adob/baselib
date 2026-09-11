export module lib.type_id;

// Preserve compatibility with declarations in the remaining headers.
export extern "C++" {
namespace lib {
    using TypeID = void*;

    namespace libdetail {
        template <typename T>
        inline constexpr char type = 0;
    }

    template <typename T>
    inline constexpr TypeID type_id = (TypeID) &libdetail::type<T>;
}
}
