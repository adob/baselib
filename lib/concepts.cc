export module lib.concepts;
import <cstddef>;
import <type_traits>;
import <concepts>;

// Preserve compatibility with declarations in the remaining headers.
export extern "C++" {
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
}
