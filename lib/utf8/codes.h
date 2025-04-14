#pragma once
#include "lib/types.h"
#include "utf8.h"

namespace lib::utf8::internal {

    const rune SurrogateMin = 0xD800;
    const rune SurrogateMax = 0xDFFF;
    
    const byte T1 = 0x00; // 0000 0000
    const byte TX = 0x80; // 1000 0000
    const byte T2 = 0xC0; // 1100 0000
    const byte T3 = 0xE0; // 1110 0000
    const byte T4 = 0xF0; // 1111 0000
    const byte T5 = 0xF8; // 1111 1000
    
    const byte MaskX = 0x3F; // 0011 1111
    const byte Mask2 = 0x1F; // 0001 1111
    const byte Mask3 = 0x0F; // 0000 1111
    const byte Mask4 = 0x07; // 0000 0111
    
    const rune Rune1Max = (1<<7)  - 1;
    const rune Rune2Max = (1<<11) - 1;
    const rune Rune3Max = (1<<16) - 1;

    const uint8

        t1 = 0b00000000,
        tx = 0b10000000,
        t2 = 0b11000000,
        t3 = 0b11100000,
        t4 = 0b11110000,
        t5 = 0b11111000,

        maskx = 0b00111111,
        mask2 = 0b00011111,
        mask3 = 0b00001111,
        mask4 = 0b00000111,

	    // The default lowest and highest continuation byte.
        locb = 0b1000'0000,
        hicb = 0b1011'1111,

        // These names of these constants are chosen to give nice alignment in the
        // table below. The first nibble is an index into acceptRanges or F for
        // special one-byte cases. The second nibble is the Rune length or the
        // Status for the special one-byte case.
        xx = 0xF1,  // invalid: size 1
        as = 0xF0,  // ASCII: size 1
        s1 = 0x02,  // accept 0, size 2
        s2 = 0x13,  // accept 1, size 3
        s3 = 0x03,  // accept 0, size 3
        s4 = 0x23,  // accept 2, size 3
        s5 = 0x34,  // accept 3, size 4
        s6 = 0x04,  // accept 0, size 4
        s7 = 0x44;  // accept 4, size 4

    const rune
        surrogate_min = 0xD800,
        surrogate_max = 0xDFFF,

        rune1_max = (1<<7) - 1,
        rune2_max = (1<<11) - 1,
        rune3_max = (1<<16) - 1;

    const byte
        rune_error_byte_0 = t3 | (RuneError >> 12),
        rune_error_byte_1 = tx | (RuneError>>6)&maskx,
        rune_error_byte_2 = tx | RuneError&maskx;
}
