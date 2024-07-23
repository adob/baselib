#pragma once

#include "lib/types.h"
#include "lib/str.h"
#include "lib/buf.h"
#include "lib/io.h"
    
namespace lib::utf8 {
    /// encode(r, b) writes into b the UTF-8 encoding
    /// of the rune r. The resulting string is returned. If b is not large enough, an exception
    /// is raised. No more than UTFMax bytes are written into b.
    str encode(rune r, buf b);
    
    /// encode(r, alloc) allocates a string using the allocator alloc, writes the UTF-8
    /// encoding of the rune r, and returns the resulting string.
    //str encode(rune r, Allocator& alloc);

    // encoding
    int encode(io::OStream &out, rune r, error &err);

    struct Encoder : io::WriterTo {
        array<const wchar_t> data;

        Encoder(array<const wchar_t> data) : data(data) {}

        void write_to(io::OStream &out, error &err) const override;
    };
}
    
