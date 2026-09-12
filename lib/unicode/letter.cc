module;
#include "letter_impl.h"

export module lib.unicode.letter;
export import lib.types;
export import lib.unicode.tables;

export extern "C++" {
namespace lib::unicode {
        
    bool is_excluding_latin(RangeTable const& rangetab, rune r);
    
}
}
