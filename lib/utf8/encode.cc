#include "utf8.h"
#include "codes.h"
#include "encode.h"

#include "lib/panic.h"

using namespace lib;
using namespace lib::utf8;
using namespace lib::utf8::internal;

int encode_rune_non_ascii(buf p, rune r) {
    // Negative values are erroneous. Making it unsigned addresses the problem.
	uint32 i = uint32(r);
    if (i <= Rune2Max) {
        p[1]; // eliminate bounds checks
        p[0] = t2 | byte(r>>6);
        p[1] = tx | byte(r)&maskx;
        return 2;
    }
    if (i < SurrogateMin || SurrogateMax < i && i <= Rune3Max) {
            p[2]; // eliminate bounds checks
            p[0] = t3 | byte(r>>12);
            p[1] = tx | byte(r>>6)&maskx;
            p[2] = tx | byte(r)&maskx;
            return 3;
    }
    if (i > Rune3Max && i <= MaxRune) {
            p[3]; // eliminate bounds checks
            p[0] = t4 | byte(r>>18);
            p[1] = tx | byte(r>>12)&maskx;
            p[2] = tx | byte(r>>6)&maskx;
            p[3] = tx | byte(r)&maskx;
            return 4;
    }
        
    p[2]; // eliminate bounds checks
    p[0] = rune_error_byte_0;
    p[1] = rune_error_byte_1;
    p[2] = rune_error_byte_2;
    return 3;
}

    
int utf8::encode_rune(buf p, rune r) {
    // This function is inlineable for fast handling of ASCII.
	if (uint32(r) <= Rune1Max) {
		p[0] = byte(r);
		return 1;
	}
	return encode_rune_non_ascii(p, r);
}
    
str utf8::encode(buf b, rune r) {
    int n = encode_rune(b, r);
    return b[0,n];
    // if (r <= Rune1Max) {
    //     if (len(b) < 1)
    //         panic("Buffer too small");
    //     b[0] = r;
    //     return b[0, 1];
    // } else if (r <= Rune2Max) {
    //     if (len(b) < 2)
    //         panic("Buffer too small");
    //     b[0] = T2 | (r >> 6);
    //     b[1] = TX | (r & MaskX);
    //     return b[0, 2];
    // } else if (r > MaxRune || (SurrogateMin <= r && r <= SurrogateMax)) {
    //     r = RuneError;
    // } else if (r <= Rune3Max) {
    //     if (len(b) < 3)
    //         panic("Buffer too small");
    //     b[0] = T3 | (r >> 12);
    //     b[1] = TX | ((r >> 6) & MaskX);
    //     b[2] = TX | (r & MaskX);
    //     return b[0, 3];
    // }
    
    // if (len(b) < 4)
    //     panic("Buffer too small");
    
    // b[0] = T4 | (r >> 18);
    // b[1] = TX | ((r >> 12) & MaskX);
    // b[2] = TX | ((r >> 6) & MaskX);
    // b[3] = TX | (r & MaskX);
    // return b[0, 4];
}


int utf8::encode(io::Writer &out, rune r, error err) {
    if (r <= rune1_max) {
        out.write(byte(r), err);
        return 1;
    }
	if (r <= rune2_max) {
		out.write(t2 | byte(r>>6), err);
		out.write(tx | (byte(r)&maskx), err);
		return 2;
    }

	if (r > MaxRune || (surrogate_min <= r && r <= surrogate_max)) {
		r = RuneError;
    }

    if (r <= rune3_max) {
		out.write(t3 | byte(r>>12), err);
		out.write(tx | (byte(r>>6)&maskx), err);
		out.write(tx | (byte(r)&maskx), err);
		return 3;
    }

    out.write(t4 | byte(r>>18), err);
    out.write(tx | (byte(r>>12)&maskx), err);
    out.write(tx | (byte(r>>6)&maskx), err);
    out.write(tx | (byte(r)&maskx), err);

    return 4;
}

void Encoder::write_to(io::Writer &out, error err) const {
    for (wchar_t c : data) {
        encode(out, c, err);
        if (err) {
            return;
        }
    }
}
