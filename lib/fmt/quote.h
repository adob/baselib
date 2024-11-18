#pragma once
#include <string>
#include "lib/str.h"
#include "lib/io.h"

namespace lib::fmt {
    struct StrQuoter : io::WriterTo {
        str s;
        bool ascii_only;
        
        StrQuoter(str s, bool ascii_only) : s(s), ascii_only(ascii_only) {}
        void write_to(io::OStream &out, error err) const override;
    };
    
    struct RuneQuoter : io::WriterTo {
        rune r;
        bool ascii_only;
        
        RuneQuoter(rune r, bool ascii_only) : r(r), ascii_only(ascii_only) {}
        void write_to(io::OStream &out, error err) const override;
    };

//     struct StreamQuoter : io::OStream, io::WriterTo {
//         WriterTo const& writer;
//
//         size direct_write(str, error&) override {
//
//         }
//
//         void write_to(io::OStream &out, error &err) const override {
//
//         }
//     } ;
    
    StrQuoter quote(str s, bool ascii_only=false);
    RuneQuoter quote(rune r, bool ascii_only=false);
    
    
    // IsPrint reports whether the rune is defined as printable by Go, with
    // the same definition as unicode.IsPrint: letters, numbers, punctuation,
    // symbols and ASCII space.
    bool is_printable(rune r);
}
