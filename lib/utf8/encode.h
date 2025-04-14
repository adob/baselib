#pragma once

#include "lib/types.h"
#include "lib/str.h"
#include "lib/buf.h"
#include "lib/io.h"
    
namespace lib::utf8 {
    /// encode(b, r) writes into b the UTF-8 encoding
    /// of the rune r. The resulting string is returned. If b is not large enough, an exception
    /// is raised. No more than UTFMax bytes are written into b.
    str encode(buf b, rune r);

    // encode_rune writes into p (which must be large enough) the UTF-8 encoding of the rune.
    // If the rune is out of range, it writes the encoding of [RuneError].
    // It returns the number of bytes written.
    int encode_rune(buf p, rune r);
    
    /// encode(r, alloc) allocates a string using the allocator alloc, writes the UTF-8
    /// encoding of the rune r, and returns the resulting string.
    //str encode(rune r, Allocator& alloc);

    // encoding
    int encode(io::Writer &out, rune r, error err);

    struct Encoder : io::WriterTo {
        arr<const wchar_t> data;

        Encoder(arr<const wchar_t> data) : data(data) {}

        void write_to(io::Writer &out, error err) const override;
    };
}
    
