export module lib.unicode;
import lib.types;


export extern "C++" {
namespace lib::unicode {
    inline constexpr rune MaxRune = 0x0010FFFF;
    inline constexpr rune ReplacementChar = 0xFFFD;
    inline constexpr rune MaxASCII  = 0x7F;
    inline constexpr rune MaxLatin1 = 0xFF;

}
}
