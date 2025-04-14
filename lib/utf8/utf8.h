#pragma once

#include "lib/io/io_stream.h"
#include "lib/types.h"
#include "lib/io.h"

/// namespace utf8 implements functions and constants to support text encoded in UTF-8. 
/// It includes functions to translate between runes and UTF-8 byte sequences.
namespace lib::utf8 {
    
    const rune RuneError = 0xFFFD;     ///< the "error" Rune or "Unicode replacement character"
    const rune RuneSelf  = 0x80;       ///< characters below Runeself are represented as themselves in a single byte.
    const rune MaxRune   = 0x0010FFFF; ///< maximum valid Unicode code point.
    const rune UTFMax    = 4;          ///< maximum number of bytes of a UTF-8 encoded Unicode character. 
    
    /// rune_start(c) reports wheather the byte c could be the first byte of an encoded rune.
    /// Second and subsequent bytes always have the top two bits set to 10.
    inline bool rune_start(char c) { return (c & 0xC0) != 0x80; }        
    
    // rune_count returns the number of runes in s. Erroneous and short
    // encodings are treated as single runes of width 1 byte.
    size rune_count(str s);
    size rune_count(io::WriterTo const&);
    
    // valid_rune reports whether r can be legally encoded as UTF-8.
    // Code points that are out of range or a surrogate half are illegal.
    bool valid_rune(rune r);

    // rune_len returns the number of bytes in the UTF-8 encoding of the rune.
    // It returns -1 if the rune is not a valid value to encode in UTF-8.
    int rune_len(rune r);

    struct AcceptRange {
        uint8 lo;
        uint8 hi;
    };
    
    struct RuneDecoder {        
        int state = 0;
        int runecount = 0;
        
        int sz;
        AcceptRange accept;

        Array<byte, 3> buffer = {{}};
        
        size count(str s);
        size decode(str s, size limit, io::Writer &w, error err);
        void decode_eof(size limit, io::Writer &w, error err);
        size eof();

        //  decode rune
        byte c0 = 0, c1 = 0, c2 = 0;
        rune decode_rune(str s, bool eof, int &bytes_consumed, bool &ok, bool &is_valid);
    };

    struct RuneCountingForwarder : io::Writer {
        io::Writer *out;
        size count();
        
        RuneCountingForwarder()                 : out(nil) {}
        RuneCountingForwarder(io::Writer &out) : out(&out) {};
        
        size direct_write(str data, error err) override;
        
      private:
          RuneDecoder state;
    };

    struct RuneTruncater : /*io::Writer, */io::WriterTo {
        io::WriterTo const *w = nil;
        // size count = 0;
        size max_count = 0;

        RuneTruncater() {}
        RuneTruncater(io::WriterTo const &w, size max_count) : w(&w), max_count(max_count)  {}
        // size direct_write(str data, error err) override;

        void write_to(io::Writer &w, error err) const override;

      private:
        RuneDecoder state;
    } ;

    // rune_truncates w after n runes have been read
    RuneTruncater rune_truncate(io::WriterTo const &w, size n);
}
    
