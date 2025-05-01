#include "lib/io/io.h"
#include "utf8.h"
#include "codes.h"
#include "decode.h"


using namespace lib;
using namespace lib::utf8;
using namespace lib::utf8::internal;

        
static rune decode_rune_i(str s, int& runesize, bool& isshort) {
    size n = len(s);
    rune r;
    if (n < 1) {
        return runesize=0, isshort=true, RuneError;
    }
    byte c0 = s[0];
    
    // 1-byte, 7-bit sequence
    if (c0 < TX) {
        return runesize=1, isshort=false, c0;
    }
    
    // unexpected continuation byte?
    if (c0 < T2) {
        return runesize=1, isshort=false, RuneError;
    }
    
    // need first continuation byte
    if (n < 2) {
        return runesize=1, isshort=true, RuneError;
    }
    byte c1 = s[1];;
    if (c1 < TX || T2 <= c1) {
        return runesize=1, isshort=false, RuneError;
    }
    
    // 2-byte, 11-bit sequence
    if (c0 < T3) {
        r = ((c0 & Mask2) << 6) | (c1 & MaskX);
        if (r <= Rune1Max) {
            return runesize=1, isshort=false, RuneError;
        }
        return runesize=2, isshort=false, r;
    }
    
    // need a third continuation byte
    if (n < 3) {
        return runesize=1, isshort=true, RuneError;
    }
    byte c2 = s[2];
    if (c2 < TX || T2 <= c2) {
        return runesize=1, isshort=false, RuneError;
    }
    
    // 3-byte, 16-bit sequence?
    if (c0 < T4) {
        r = ((c0 & Mask3) << 12) | ((c1 & MaskX) << 6) | (c2 & MaskX);
        if (r <= Rune2Max) {
            return runesize=1, isshort=false, RuneError;
        }
        if (SurrogateMin <= r && r <= SurrogateMax) {
            return runesize=1, isshort=false, RuneError;
        }
        return runesize=3, isshort=false, r;
    }
    
    // need a third continuation byte
    if (n < 4) {
        return runesize=1, isshort=true, RuneError;
    }
    byte c3 = s[3];
    if (c3 < TX || T2 <= c3) {
        return runesize=1, isshort=true, RuneError;
    }
    
    // 4-byte, 21-bit sequence?
    if (c0 < T5) {
        r = ((c0 & Mask4) << 18) | ((c1 & MaskX) << 12) | ((c2 & MaskX) << 6) | (c3 & MaskX);
        if (r <= Rune3Max || MaxRune < r) {
            return runesize=1, isshort=true, RuneError;
        }
        return runesize=4, isshort=false, r;
    }
    
    // error
    return runesize=1, isshort=false, RuneError;
}

rune utf8::decode_rune(str s, int& nbytes) {
    bool isshort;
    return decode_rune_i(s, nbytes, isshort);
}

DecodeRuneResult utf8::decode_rune(str s) {
    DecodeRuneResult result;
    bool isshort;
    result.r = decode_rune_i(s, result.size, isshort);
    return result;
}

rune utf8::decode_last_rune(str s, int& nbytes) {
    size end = len(s);
    if (end == 0) {
        return nbytes=0, RuneError;
    }
    size start = end - 1;
    rune r = s[start];
    if (r < RuneSelf) {
        return nbytes=1, r;
    }
    // guard against O(n^2) behavior when traversing
    // backwards through strings with long sequences of
    // invalid UTF-8.
    size lim = end - UTFMax;
    if (lim < 0) {
        lim = 0;
    }
    for (start--; start >= lim; start--) {
        if (rune_start(s[start])) {
            break;
        }
    }
    if (start < 0) {
        start = 0;
    }
    r = decode_rune(s[start, end], nbytes);
    if (start + nbytes != end) {
        return nbytes=1, RuneError;
    }
    return r;
}

DecodeRuneResult utf8::decode_last_rune(str s) {
    DecodeRuneResult result;
    result.r = decode_last_rune(s, result.size);
    return result;
}

// IterableStr
void IterableStr::Iterator::operator ++ () {
    index += rune_len;
    s = s+rune_len;
    r = decode_rune(s, rune_len);
}

std::pair<size, rune> IterableStr::Iterator::operator * () {
    return {index, r};
}

bool IterableStr::Iterator::operator != (Iterator const&) {
    return len(s) != 0;
}

IterableStr::Iterator IterableStr::begin() const {
    IterableStr::Iterator ri{s, 0};
    ++ri;
    return ri;
}

IterableStr::Iterator IterableStr::end() const {
    return IterableStr::Iterator{{}, 0};
}

bool utf8::full_rune(str s) {
    bool isshort;
    int nbytes;
    decode_rune_i(s, nbytes, isshort);
    return !isshort;
}

IterableStr utf8::runes(str s) { 
    return IterableStr(s); 
}
