#pragma  once
#include "tables.h"

namespace lib::unicode {
        
    bool is_excluding_latin(RangeTable const& rangetab, rune r);
    
}