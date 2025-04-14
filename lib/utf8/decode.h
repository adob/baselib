#pragma once

#include "lib/array.h"
#include "lib/types.h"
#include "lib/str.h"
#include "lib/io/io_stream.h"

namespace lib::utf8 {
    struct DecodeRuneResult {
        rune r;
        int size;
    } ;

    /// decode_rune(s[, nbytes]) unpacks the first UTF-8 encoding in s and returns the rune with
    /// its width in bytes stored in nbytes, if present. If the encoding is invalid, RuneError
    /// is returned and nbytes is 1, an impossible result for correct UTF-8.
    /// An encoding is invlalid if it is incorrect UTF-8, encodes a rune that is out
    /// range, or is not the shortest possible UTF-8 encoding for the value.
    /// No other validation is performed.
    rune decode_rune(str s, int& nbytes);
    DecodeRuneResult decode_rune(str s);
    
    /// decode_last(s[, nbytes]) unpacks the last UTF-8 encoding in s and return the rune
    /// with its width in bytes stored in nbytes, if present.
    /// If the encoding is invalid, RuneError
    /// is returned and nbytes is 1, an impossible result for correct UTF-8.
    /// An encoding is invlalid if it is incorrect UTF-8, encodes a rune that is out
    /// range, or is not the shortest possible UTF-8 encoding for the value.
    /// No other validation is performed.
    rune decode_last_rune(str s, int& nbytes);
    DecodeRuneResult decode_last_rune(str s);
    
    /// full_rune(s) reports whether the bytes in s begin with a full UTF-8 encoding of a  rune.
    /// An invalid encoding is considered a full rune since it will convert as a width-1 error rune.
    bool full_rune(str s);
    
    struct IterableStr {
        str s;
        
        struct Iterator {
            str          s;
            rune         r;
            int          rune_len = 0;
            size         index = 0;
            
            void operator ++ ();
            std::pair<size, rune> operator * ();
            bool operator != (Iterator const&);
        } ;
        
        Iterator begin() const;
        Iterator end() const;
    } ;

    IterableStr runes(str s);
}
