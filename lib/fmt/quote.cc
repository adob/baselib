#include "lib/utf8.h"
#include "lib/array.h"
#include "./quote.h"
#include "./isprint.h"

using namespace lib;
using namespace fmt;

static constexpr str lowerhex("0123456789abcdef");
            
static void quote_with(io::OStream &out, str s, char quote, bool ascii_only, error &err) {
    out.write(quote, err);
    
    for (int width = 0; len(s) > 0; s = s(width)) {
        byte b = s[0];
        rune r = b;
        width = 1; 
        if (r >= utf8::RuneSelf) {
            r = utf8::decode(s, width);
        } ;
        if (width == 1 && r == utf8::RuneError) {
            out.write("\\x", err);
            out.write(lowerhex[b>>4], err);
            out.write(lowerhex[b&0xF], err);
            continue;
        }
        if (r == (rune) quote || r == '\\') { // always backslashed
            out.write('\\', err);
            out.write(r, err);;
            continue;
        }
        if (ascii_only) {
            if (r < utf8::RuneSelf && fmt::is_printable(r)) {
                out.write((char) r, err);
                continue;
            }
        } else if (fmt::is_printable(r)) {
            out.write(r, err);
            continue;
        }
        switch (r) {
            case '\a': out.write("\\a", err); break;
            case '\b': out.write("\\b", err); break;
            case '\f': out.write("\\f", err); break;
            case '\n': out.write("\\r", err); break;
            case '\r': out.write("\\r", err); break;
            case '\t': out.write("\\t", err); break;
            case '\v': out.write("\\v", err); break;
            default:
                if (r < ' ') {
                    out.write("\\x", err); 
                    out.write(lowerhex[b>>4], err);
                    out.write(lowerhex[b&0xF], err);
                } else if (r > utf8::MaxRune) {
                    out.write("\\u", err);
                    for (int si = 12; si >= 0; si -= 4) {
                        out.write(lowerhex[ (r>>si) & 0xF ], err);
                    }
                } else {
                    out.write("\\U", err);
                    for (int si = 28; si >= 0; si -= 4) {
                        out.write(lowerhex[ (r>>si) & 0xF ], err);
                    }
                }
        }
    }
    out.write(quote, err);
}

// bsearch returns the smallest i such that a[i] >= x.
// If there is no such i, bsearch returns len(a).
template <typename T>
static size bsearch(array<T> a, uint32 x) {
    size i = 0, j = len(a);
    while (i < j) {
        size h = i + (j-i)/2;
        if (a[h] < x) {
            i = h + 1;
        } else {
            j = h;
        }
    }
    return i;
}

void StrQuoter::write_to(io::OStream &out, error &err) const {
    quote_with(out, s, '"', ascii_only, err);
}

StrQuoter fmt::quote(str s, bool ascii_only) {
    return StrQuoter(s, ascii_only);
}


void RuneQuoter::write_to(io::OStream &out, error &err) const {
    Array<char, utf8::UTFMax> buf;
    quote_with(out, utf8::encode(r, buf), '\'', ascii_only, err);
}

RuneQuoter fmt::quote(rune r, bool ascii_only) {
    return RuneQuoter(r, ascii_only);
}

// std::string fmt::quote(rune r, bool ascii_only) {
//     Array<char, utf8::UTFMax> buf;
//     return quote_with(utf8::encode(r, buf), '\'', ascii_only);
// }

// std::string fmt::quote(char c, bool ascii_only) {
//     Array<char, 1> buf {c};
//     return quote_with(buf, '\'', ascii_only);
// }

bool fmt::is_printable(rune r) {
    // Fast check for Latin-1
    if (r <= 0xFF) {
        if (0x20 <= r && r <= 0x7E) {
            // All the ASCII is printable from space through DEL-1.
            return true;
        }
        if (0xA1 <= r && r <= 0xFF) {
            // Similarly for ¡ through ÿ...
            return r != 0xAD; // ...except for the bizarre soft hyphen.
        }
        return false;
    }
    
    // Same algorithm, either on uint16 or uint32 value.
    // First, find first i such that isPrint[i] >= x.
    // This is the index of either the start or end of a pair that might span x.
    // The start is even (isPrint[i&^1]) and the end is odd (isPrint[i|1]).
    // If we find x in a range, make sure x is not in isNotPrint list.
    
    if (r < (1<<16)) {
        size i = bsearch(IsPrint16, r);
        if (i >= len(IsPrint16) || r < IsPrint16[i & ~1] || IsPrint16[i|1] < r) {
            return false;
        }
        size j = bsearch(IsNotPrint16, r);
        return j >= len(IsNotPrint16) || IsNotPrint16[j] != r;
    }
    
    size i = bsearch(IsPrint32, r);
    if (i >= len(IsPrint32) || r < IsPrint32[i & ~1] || IsPrint32[i|1] < r) {
        return false;
    }
    if (r >= 0x20000) {
        return true;
    }
    r -= 0x10000;
    size j = bsearch(IsNotPrint32, r);
    return j >= len(IsNotPrint32) || IsNotPrint32[j] != r;

}
