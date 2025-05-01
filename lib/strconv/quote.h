#pragma once

#include "lib/str.h"
#include "lib/io/io.h"

namespace lib::strconv {
    namespace detail {
        inline const char *lowerhex = "0123456789abcdef";
        inline const char *upperhex = "0123456789ABCDEF";
    }
    // can_backquote reports whether the string s can be represented
    // unchanged as a single-line backquoted string without control
    // characters other than tab.
    bool can_backquote(str s);
    bool can_backquote(io::WriterTo const &w);

    struct Quoter : io::WriterTo {
        str s;
        bool ascii_only;
        
        Quoter(str s, bool ascii_only) : s(s), ascii_only(ascii_only) {}
        void write_to(io::Writer &out, error err) const override;
    };
    
    struct RuneQuoter : io::WriterTo {
        rune r;
        bool ascii_only;
        
        RuneQuoter(rune r, bool ascii_only) : r(r), ascii_only(ascii_only) {}
        void write_to(io::Writer &out, error err) const override;
    };

    struct WriterToQuoter : io::WriterTo {
        io::WriterTo const *w = nil;
        bool ascii_only = false;

        void write_to(io::Writer &out, error err) const override;
    } ;

    // quote returns a double-quoted Go string literal representing s. The
    // returned string uses Go escape sequences (\t, \n, \xFF, \u0100) for
    // control characters and non-printable characters as defined by
    // [IsPrint].
    Quoter quote(str s, bool ascii_only=false);
    RuneQuoter quote(rune r, bool ascii_only=false);
    WriterToQuoter quote(io::WriterTo const &w, bool ascii_only = false);

    // quote_rune returns a single-quoted Go character literal representing the
    // rune. The returned string uses Go escape sequences (\t, \n, \xFF, \u0100)
    // for control characters and non-printable characters as defined by [IsPrint].
    // If r is not a valid Unicode code point, it is interpreted as the Unicode
    // replacement character U+FFFD.
    RuneQuoter quote_rune(rune r);

    // quote_to_ascii returns a double-quoted Go string literal representing s.
    // The returned string uses Go escape sequences (\t, \n, \xFF, \u0100) for
    // non-ASCII characters and non-printable characters as defined by [IsPrint].
    Quoter quote_to_ascii(str s);
    WriterToQuoter quote_to_ascii(io::WriterTo const &w);

    // quote_rune_to_ascii returns a single-quoted Go character literal representing
    // the rune. The returned string uses Go escape sequences (\t, \n, \xFF,
    // \u0100) for non-ASCII characters and non-printable characters as defined
    // by [IsPrint].
    // If r is not a valid Unicode code point, it is interpreted as the Unicode
    // replacement character U+FFFD.
    RuneQuoter quote_rune_to_ascii(rune r);

    // is_print reports whether the rune is defined as printable by Go, with
    // the same definition as unicode.IsPrint: letters, numbers, punctuation,
    // symbols and ASCII space.
    bool is_print(rune r);
}