#include "strings.h"
#include "../unicode.h"

using namespace lib;
using namespace strings;

namespace {
                
    // PrimeRK is the prime base used in Rabin-Karp algorithm.
    const uint PrimeRK = 16777619;

    // hashstr returns the hash and the appropriate multiplicative
    // factor for use in Rabin-Karp algorithm.
    void hashstr(str sep, uint32& out_hash, uint32& out_pow) {
        uint32 hash = 0;
        for (uint8 c : sep) {
            hash = hash*PrimeRK + c;
        }
        uint32 pow = 1, sq = PrimeRK;
        for (size i = sep.len; i > 0; i >>= 1) {
            if ((i&1) != 0) {
                pow *= sq;
            }
            sq *= sq;
        }
        
        return out_hash=hash, out_pow=pow, (void) 0;
    }
} // namespace


// Index returns the index of the first instance of needle in haystack, or -1 if needle is not present in haystack.
size strings::index(str haystack, str needle) {
    if (needle.len == 0) {
        return 0;
    } else if (needle.len == 1) {
        return index(haystack, needle[0]);
    } else if (haystack.len == needle.len) {
        return haystack == needle ? 0 : -1;
    } else if (needle.len > haystack.len) {
        return -1;
    }
    
    uint32 hashsep, pow;
    hashstr(needle, hashsep, pow);
    uint32 h = 0;
    for (char c : haystack[0, len(needle)]) {
        h = h*PrimeRK + uint(c);
    }
    
    if (h == hashsep && haystack[0, needle.len] == needle) {
        return 0;
    }
    
    for (size i = needle.len; i < haystack.len; ) {
        h *= PrimeRK;
        h += (uint32) haystack[i];
        h -= pow * (uint32) haystack[i-needle.len];
        i++;
        if (h == hashsep && haystack[i-needle.len,i] == needle) {
            return i - needle.len;
        }
    }
    return -1;
}


// String strings::to_lower_ascii(str s) {
//     const char *siter = begin(s);
//     const char *send  = end(s);
    
//     for (; siter != send; ++siter) {
//         char C = *siter;
//         char c = char(tolower(C));
//         if (c != C) {
//             // must allocate
//             //bufref b = alloc(len(s));
//             size len = siter-begin(s);
//             memcpy(begin(b), begin(s), len);
            
//             byte *biter = begin(b) + len;
//             for (; siter != send; ++siter, ++biter) {
//                 *biter = tolower(*siter);
//             }
            
//             return b;
//         }
//     }

//     return s;
// }

str strings::rtrim(str s, str cutset) {
    if (len(s) == 0 || len(cutset) == 0)
        return s;
    return rtrim(s, [=] (rune r) -> bool {
        return index(cutset, r) != -1;
    });
}

str strings::ltrim(str s, str cutset) {
    if (len(s) == 0 || len(cutset) == 0)
        return s;
    return ltrim(s, [=] (rune r) -> bool {
        return index(cutset, r) != -1;
    });
}

str trim(str s, str cutset) {
    if (len(s) == 0 || len(cutset) == 0)
        return s;
    return trim(s, [=] (rune r) -> bool {
        return index(cutset, r) != -1;
    });
}

str strings::trim(str s) {
    return trim(s, unicode::is_space);
}

str strings::ltrim(str s) {
    return ltrim(s, unicode::is_space);
}

str strings::rtrim(str s) {
    return rtrim(s, unicode::is_space);
}