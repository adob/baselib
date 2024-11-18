#include "utf8.h"
#include "codes.h"
#include "encode.h"

#include "lib/panic.h"

using namespace lib;
using namespace lib::utf8;

namespace lib::utf8 {
    using namespace utf8::internal;
    
    str encode(rune r, buf b) {
        if (r <= Rune1Max) {
            if (len(b) < 1)
                panic("Buffer too small");
            b[0] = r;
            return b[0, 1];
        } else if (r <= Rune2Max) {
            if (len(b) < 2)
                panic("Buffer too small");
            b[0] = T2 | (r >> 6);
            b[1] = TX | (r & MaskX);
            return b[0, 2];
        } else if (r > MaxRune || (SurrogateMin <= r && r <= SurrogateMax)) {
            r = RuneError;
        } else if (r <= Rune3Max) {
            if (len(b) < 3)
                panic("Buffer too small");
            b[0] = T3 | (r >> 12);
            b[1] = TX | ((r >> 6) & MaskX);
            b[2] = TX | (r & MaskX);
            return b[0, 3];
        }
        
        if (len(b) < 4)
            panic("Buffer too small");
        
        b[0] = T4 | (r >> 18);
        b[1] = TX | ((r >> 12) & MaskX);
        b[2] = TX | ((r >> 6) & MaskX);
        b[3] = TX | (r & MaskX);
        return b[0, 4];
    }
    
//     str encode(rune r, Allocator& alloc) {
//         bufref b = alloc(UTFMax);
//         return encode(r, b);
//     }
}

int utf8::encode(io::OStream &out, rune r, error err) {
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

void Encoder::write_to(io::OStream &out, error err) const {
    for (wchar_t c : data) {
        encode(out, c, err);
        if (err) {
            return;
        }
    }
}
