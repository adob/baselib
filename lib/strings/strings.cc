#include "strings.h"
#include "2way.h"
#include "../unicode.h"
#include "lib/math/math.h"
#include "lib/print.h"
#include "lib/strings/2way.h"
#include "lib/utf8/decode.h"
#include <vector>

using namespace lib;
using namespace lib::strings;

// Optimized memmem function.
//    Copyright (c) 2018 Arm Ltd.  All rights reserved.

//    SPDX-License-Identifier: BSD-3-Clause

//    Redistribution and use in source and binary forms, with or without
//    modification, are permitted provided that the following conditions
//    are met:
//    1. Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//    2. Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//    3. The name of the company may not be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.

//    THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED
//    WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//    IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//    TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 


/* Fast memmem algorithm with guaranteed linear-time performance.
   Small needles up to size 2 use a dedicated linear search.  Longer needles
   up to size 256 use a novel modified Horspool algorithm.  It hashes pairs
   of characters to quickly skip past mismatches.  The main search loop only
   exits if the last 2 characters match, avoiding unnecessary calls to memcmp
   and allowing for a larger skip if there is no match.  A self-adapting
   filtering check is used to quickly detect mismatches in long needles.
   By limiting the needle length to 256, the shift table can be reduced to 8
   bits per entry, lowering preprocessing overhead and minimizing cache effects.
   The limit also implies worst-case performance is linear.
   Needles larger than 256 characters use the linear-time Two-Way algorithm.  
   
   https://github.com/Alexpux/Cygwin/blob/master/newlib/libc/string/memmem.c
   */

namespace {
    // Hash character pairs so a small shift table can be used.  All bits of
    // p[0] are included, but not all bits from p[-1].  So if two equal hashes
    // match on p[-1], p[0] matches too.  Hash collisions are harmless and result
    // in smaller shifts.
    inline uint8 hash2(const byte *p) {
        return uint8(usize(p[0]) - (usize(p[-1]) << 3));
    }

    inline uint8 hash2r(const byte *p) {
        return uint8(usize(p[0]) - (usize(p[1]) << 3));
    }
    
} // namespace

// https://github.com/bminor/newlib/blob/master/newlib/libc/string/memmem.c
// https://github.com/ashvardanian/StringZilla

size strings::index(str haystack, str needle) {
    size n = len(needle);
    if (n == 0) {
        return 0;
    } 
    
    if (n == 1) {
        return index_byte(haystack, needle[0]);
    }
    
    if (n == len(haystack)) {
        return haystack == needle ? 0 : -1;
    }
    
    if (n > len(haystack)) {
        return -1;
    }

    if (n == 2) {
        uint32 nw = ((uint32)needle[0] << 16) | needle[1];
	    uint32 hw = ((uint32)haystack[0] << 16) | haystack[1];

        const byte *hs = (const byte *) haystack.data;
        const byte *end = hs + haystack.len - needle.len;


        for (hs++; hs <= end && hw != nw; ) {
            hw = hw << 16 | *++hs;
        }
        
        if (hw == nw) {
            return  (hs - 1) - ((const byte*) haystack.data);
        }
        return -1;
    }

    if (n > 256) {
        return detail::two_way_search_fwd(haystack, needle);
    }
    
    // Initialize bad character shift hash table.
    byte shift[256] = {0};
    byte m1 = byte(n - 1);
    const byte *ne = (const byte *) needle.data;
    for (size i = 1; i < m1; i++) {
        shift[hash2(ne+i)] = byte(i);
    }

    byte shift1 = m1 - shift[hash2(ne+m1)];
    shift[hash2(ne+m1)] = m1;    

    const byte *hs = (const byte *) haystack.data;
    const byte *end = hs + haystack.len - needle.len;
    size offset = 0;

    while (hs <= end) {
        byte tmp;
        // Skip past character pairs not in the needle.
        do {
            hs += m1;
            tmp = shift[hash2(hs)];
        } while (hs <= end && tmp == 0);

        // If the match is not at the end of the needle, shift to the end
        // and continue until we match the last 2 characters.
        hs -= tmp;
        if (tmp < m1) {
            continue;
        }

        // The last 2 characters match.  If the needle is long, check a
        // fixed number of characters first to quickly filter out mismatches.
        if (m1 <= 15 || memcmp(hs + offset, ne + offset, 8) == 0) {
            if (memcmp(hs, ne, m1) == 0) {
                return hs - (const byte *) haystack.data;
            }

            // Adjust filter offset when it doesn't find the mismatch. 
            offset = (offset >= 8 ? offset : m1) - 8;
        }

        // Skip based on matching the last 2 characters.
        hs += shift1;
    }

    return -1;
}

size strings::last_index(str haystack, str needle) {
    size n = len(needle);
    if (n == 0) {
        return len(haystack);
    } 
    
    if (n == 1) {
        return last_index_byte(haystack, needle[0]);
    }
    
    if (n == len(haystack)) {
        return haystack == needle ? 0 : -1;
    }
    
    if (n > len(haystack)) {
        return -1;
    }

    if (n == 2) {
        // print "case n == 2; needle %q; haystack %q" % needle, haystack;
        uint32 nw = ((uint32)needle[n-1] << 16) | needle[n-2];
        uint32 hw = ((uint32)haystack[len(haystack)-1] << 16) | haystack[len(haystack)-2];

        const byte *begin = (const byte *) haystack.data;
        const byte *ptr = begin + haystack.len - needle.len;
        
        while (ptr > begin && hw != nw) {
            // print "loop iteration";
            ptr--;
            hw = hw << 16 | *ptr;
        }

        if (hw == nw) {
            // print "match";
            return ptr - begin;
        }

        return -1;
    }

    if (n > 256) {
        return detail::two_way_search_rev(haystack, needle);
    }

    // Initialize bad character shift hash table.
    byte shift[256] = {0};
    byte m1 = byte(n - 1);
    const byte *ne = (const byte *) needle.data;
    for (size i = n - 2; i >= 1; i--) {
        shift[hash2r(ne + i)] = byte(n - i - 1);
    }
    byte shift1 = m1 - shift[hash2r(ne)];
    shift[hash2r(ne)] = m1;

    const byte *hs = (const byte *) haystack.data + haystack.len - 1;
    const byte *begin = (const byte *) haystack.data;
    const byte *ne_last = (const byte *)  needle.data + needle.len - 1;
    size offset = 0;

    while (hs >= begin + m1) {
        // print "loop";
        byte tmp;
        // Skip past character pairs not in the needle.
        do {
            hs -= m1;
            tmp = shift[hash2r(hs)];
        } while (hs >= begin + m1 && tmp == 0);

        // If the match is not at the end of the needle, shift to the end
        // and continue until we match the last 2 characters.
        hs += tmp;
        if (tmp < m1) {
            continue;
        }

        // print "last 2 match index %v" % (hs - begin);

        // The last 2 characters match.  If the needle is long, check a
        // fixed number of characters first to quickly filter out mismatches.
        if (m1 <= 15 || memcmp(hs - offset - 8, ne_last - offset - 8, 8) == 0 ) {
            if (memcmp(hs - m1 + 1, ne_last - m1 + 1, m1) == 0) {
                return hs - m1 - begin;
            }

            // Adjust filter offset when it doesn't find the mismatch.
            offset = (offset >= 8 ? offset : m1) - 8;
        }

        hs -= shift1;
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

// str strings::rtrim(str s, str cutset) {
//     if (len(s) == 0 || len(cutset) == 0)
//         return s;
//     return rtrim(s, [=] (rune r) -> bool {
//         return index(cutset, r) != -1;
//     });
// }

// str strings::ltrim(str s, str cutset) {
//     if (len(s) == 0 || len(cutset) == 0)
//         return s;
//     return ltrim(s, [=] (rune r) -> bool {
//         return index(cutset, r) != -1;
//     });
// }

// str trim(str s, str cutset) {
//     if (len(s) == 0 || len(cutset) == 0)
//         return s;
//     return trim(s, [=] (rune r) -> bool {
//         return index(cutset, r) != -1;
//     });
// }

// str strings::trim(str s) {
//     return trim(s, unicode::is_space);
// }

// str strings::ltrim(str s) {
//     return ltrim(s, unicode::is_space);
// }

// str strings::rtrim(str s) { return rtrim(s, unicode::is_space); }

bool strings::contains_rune(str s, rune r) {
  return index_rune(s, r) >= 0;
}
// lib::size lib::strings::index_rune(str s, rune r) {
//     // const haveFastIndex = bytealg.MaxBruteForce > 0
// 	if (0 <= r && r < utf8::RuneSelf) {
// 		return IndexByte(s, byte(r))
//     } case r == utf8.RuneError:
// 		for i := 0; i < len(s); {
// 			r1, n := utf8.DecodeRune(s[i:])
// 			if r1 == utf8.RuneError {
// 				return i
// 			}
// 			i += n
// 		}
// 		return -1
// 	case !utf8.ValidRune(r):
// 		return -1
// 	default:
// 		// Search for rune r using the last byte of its UTF-8 encoded form.
// 		// The distribution of the last byte is more uniform compared to the
// 		// first byte which has a 78% chance of being [240, 243, 244].
// 		var b [utf8.UTFMax]byte
// 		n := utf8.EncodeRune(b[:], r)
// 		last := n - 1
// 		i := last
// 		fails := 0
// 		for i < len(s) {
// 			if s[i] != b[last] {
// 				o := IndexByte(s[i+1:], b[last])
// 				if o < 0 {
// 					return -1
// 				}
// 				i += o + 1
// 			}
// 			// Step backwards comparing bytes.
// 			for j := 1; j < n; j++ {
// 				if s[i-j] != b[last-j] {
// 					goto next
// 				}
// 			}
// 			return i - last
// 		next:
// 			fails++
// 			i++
// 			if (haveFastIndex && fails > bytealg.Cutover(i)) && i < len(s) ||
// 				(!haveFastIndex && fails >= 4+i>>4 && i < len(s)) {
// 				goto fallback
// 			}
// 		}
// 		return -1

// 	fallback:
// 		// Switch to bytealg.Index, if available, or a brute force search when
// 		// IndexByte returns too many false positives.
// 		if haveFastIndex {
// 			if j := bytealg.Index(s[i-last:], b[:n]); j >= 0 {
// 				return i + j - last
// 			}
// 		} else {
// 			// If bytealg.Index is not available a brute force search is
// 			// ~1.5-3x faster than Rabin-Karp since n is small.
// 			c0 := b[last]
// 			c1 := b[last-1] // There are at least 2 chars to match
// 		loop:
// 			for ; i < len(s); i++ {
// 				if s[i] == c0 && s[i-1] == c1 {
// 					for k := 2; k < n; k++ {
// 						if s[i-k] != b[last-k] {
// 							continue loop
// 						}
// 					}
// 					return i - last
// 				}
// 			}
// 		}
// 		return -1
// 	}
// }

size lib::strings::index(str s, rune r) {
  if (r < utf8::RuneSelf) {
    return index_byte(s, (char)r);
  }

  size start = 0;
  while (start < len(s)) {
    int wid = 1;
    rune rr = rune(s[start]);
    if (rr >= utf8::RuneSelf) {
      rr = utf8::decode_rune(s + start, wid);
    }

    if (rr == r) {
      return start;
    }

    start += wid;
  }
  return -1;
}

size strings::index_rune(str s, rune r) { 
    return index(s, r); 
}

Repeater strings::repeat(str s, size count) {
    if (count < 0) {
		panic("strings: negative Repeat count");
	}

    size result_size;
    bool overflow = __builtin_mul_overflow(len(s), count, &result_size);
    if (overflow) {
        panic("strings: repeat output length overflow");
    }

    Repeater repeater;
    repeater.s = s;
    repeater.count = count;

    return repeater;
}
void Repeater::write_to(io::Writer &out, error err) const {
    for (size i = 0; i < count; i++) {
        out.write(s, err);
        if (err) {
            return;
        }
    }
}


// ASCIISet is a 32-byte value, where each bit represents the presence of a
// given ASCII character in the set. The 128-bits of the lower 16 bytes,
// starting with the least-significant bit of the lowest word to the
// most-significant bit of the highest word, map to the full range of all
// 128 ASCII characters. The 128-bits of the upper 16 bytes will be zeroed,
// ensuring that any non-ASCII character will be reported as not in the set.
// This allocates a total of 32 bytes even though the upper half
// is unused to avoid bounds checks in asciiSet.contains.
struct ASCIISet {
    std::array<uint32, 32> data = {};
    bool ok = false;

    // contains reports whether c is inside the set.
    bool contains(byte c)  const {
        return (data[c/32] & (1 << (c % 32))) != 0;
    }
} ;

// make_ascii_set creates a set of ASCII characters and reports whether all
// characters in chars are ASCII.
static ASCIISet make_ascii_set(str chars) {
    ASCIISet as;

    for (size i = 0; i < len(chars); i++) {
		byte c = chars[i];
		if (c >= utf8::RuneSelf) {
			return as.ok = false, as;
		}
		as.data[c/32] |= 1 << (c % 32);
	}

	return as.ok=true, as;
}

size strings::last_index_any(str s, str chars) {
    if (!chars) {
		// Avoid scanning all of s.
		return -1;
	}
	if (len(s) == 1) {
		rune rc = rune(s[0]);
		if (rc >= utf8::RuneSelf) {
			rc = utf8::RuneError;
		}
		if (index_rune(chars, rc) >= 0) {
			return 0;
		}
		return -1;
	}
	if (len(s) > 8) {
		if (ASCIISet as = make_ascii_set(chars); as.ok) {
			for (size i = len(s) - 1; i >= 0; i--) {
				if (as.contains(s[i])) {
					return i;
				}
			}
			return -1;
		}
	}
	if (len(chars) == 1) {
		rune rc = rune(chars[0]);
		if (rc >= utf8::RuneSelf) {
			rc = utf8::RuneError;
		}
		for (size i = len(s); i > 0;) {
			auto [r, size] = utf8::decode_last_rune(s[0,i]);
			i -= size;
			if (rc == r) {
				return i;
			}
		}
		return -1;
	}
	for (size i = len(s); i > 0;) {
		auto [r, size] = utf8::decode_last_rune(s[0,i]);
		i -= size;
		if (index_rune(chars, r) >= 0) {
			return i;
		}
	}
	return -1;
}


bool Split::has_next() { 
    // print "HAS NEXT", i, n, i < n;
    return i < n; 
}

str Split::next() {
    i++;
    if (i >= n) {
        return s;
    }
    if (len(sep) == 0) {
        int nbytes = utf8::decode_rune(s).size;
        str result = s[0,nbytes];
        s = s + nbytes;
        if (len(s) == 0) {
            n = -1;
        }
        return result;
    }
    size m = index(s, sep);
    // fmt::printf("INDEX is %v; haystack % X; needle % X\n", m, s, sep);
    if (m == -1) {
        str result = s;
        n = -1;
        return result;
        
    }
    str result = s[0, m+sep_save];
    s = s + (m+len(sep));
    return result;
}

Split strings::split(str s, str sep) {
    return Split(s, sep, 0, math::MaxSize);
}

Split strings::split_n(str s, str sep, size n) {
    if (n < 0) {
        n = math::MaxSize;
    }
    return Split( s, sep, 0, n);
}

Split strings::split_after(str s, str sep) {
    return Split(s, sep, len(sep), -1);
}

Split strings::split_after_n(str s, str sep, size n) {
    if (n < 0) {
        n = math::MaxSize;
    }
    return Split(s, sep, len(sep), n);
}

Split::Split(str s, str sep, size sep_save, size n) : s(s), sep(sep), sep_save(sep_save), n(n) {
    // fmt::printf("SPLIT %q; len %v\n", s, len(s));
    if (len(s) == 0 && sep == "") {
        this->n = -1;
    }
}

// explode splits s into a slice of UTF-8 strings,
// one string per Unicode character up to a maximum of n (n < 0 means no limit).
// Invalid UTF-8 bytes are sliced individually.
static std::vector<str> explode(str s, size n) {
	size l = utf8::rune_count(s);
	if (n < 0 || n > l) {
		n = l;
	}
	std::vector<str> a(n);
	for (size i = 0; i < n-1; i++) {
		size size = utf8::decode_rune(s).size;
		a[i] = s[0,size];
		s = s+size;
	}
	if (n > 0) {
		a[n-1] = s;
	}
	return a;
}

lib::strings::Split::operator std::vector<str>() const {
    size n = this->n;
    str s = this->s;

    if (n == 0) {
		return {};
	}

	if (sep == "") {
		return explode(s, n);
	}
	// if (n < 0) {
	// 	n = count(s, sep) + 1;
	// }

	// if (n > len(s)+1) {
	// 	n = len(s) + 1;
	// }
	std::vector<str> a;
	n--;
	size i = 0;
	while (i < n) {
		size m = index(s, sep);
		if (m < 0) {
			break;
		}
		a.push_back(s[0,m+sep_save]);
		s = s+(m+len(sep));
		i++;
	}
	a.push_back(s);
    return a;
	//return a[:i+1]
}

size strings::count(str s, str substr) {
    // special case
	if (len(substr) == 0) {
		return utf8::rune_count(s) + 1;
	}

	if (len(substr) == 1) {
        byte c = substr[0];
        size n = 0;
        for (;;) {
            size i = index_byte(s, c);
            if (i == -1) {
                return n;
            }
            n++;
            s = s+(i+1);
        }
	}

	size n = 0;
	for (;;) {
		size i = index(s, substr);
		if (i == -1) {
			return n;
		}
		n++;
		s = s + (i+len(substr));
	}
}

// size strings::last_index(str s, str substr) {
//     size n = len(substr);
// 	switch (n) {
// 	case 0:
// 		return len(s);
// 	case 1:
// 		return bytealg.LastIndexByteString(s, substr[0])
// 	case n == len(s):
// 		if substr == s {
// 			return 0
// 		}
// 		return -1
// 	case n > len(s):
// 		return -1
// 	}
// 	// Rabin-Karp search from the end of the string
// 	hashss, pow := bytealg.HashStrRev(substr)
// 	last := len(s) - n
// 	var h uint32
// 	for i := len(s) - 1; i >= last; i-- {
// 		h = h*bytealg.PrimeRK + uint32(s[i])
// 	}
// 	if h == hashss && s[last:] == substr {
// 		return last
// 	}
// 	for i := last - 1; i >= 0; i-- {
// 		h *= bytealg.PrimeRK
// 		h += uint32(s[i])
// 		h -= pow * uint32(s[i+n])
// 		if h == hashss && s[i:i+n] == substr {
// 			return i
// 		}
// 	}
// 	return -1
// }
