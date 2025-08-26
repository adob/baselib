#include "utf8.h"
#include "codes.h"
#include "lib/io/io.h"
#include "lib/math.h"

using namespace lib;
using namespace utf8;
using namespace lib::utf8::internal;
namespace {



    Array<AcceptRange, 16> accept_ranges {{
        {locb, hicb},
        {0xA0, hicb},
        {locb, 0x9F},
        {0x90, hicb},
        {locb, 0x8F},
    }};
    
    Array<uint8, 256> first {{
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x00-0x0F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x10-0x1F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x20-0x2F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x30-0x3F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x40-0x4F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x50-0x5F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x60-0x6F
        as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, as, // 0x70-0x7F
        //   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
        xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0x80-0x8F
        xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0x90-0x9F
        xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xA0-0xAF
        xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xB0-0xBF
        xx, xx, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, // 0xC0-0xCF
        s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, s1, // 0xD0-0xDF
        s2, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s3, s4, s3, s3, // 0xE0-0xEF
        s5, s6, s6, s6, s7, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, // 0xF0-0xFF
    }};
}

size RuneDecoder::count(str s) {
    // size runecount = 0;
    
    for (byte c : s) {
        // fmt::printf("LOOP %#U\n", c);
        switch (state) {
        restart:
          case 0: {
            runecount++;
            // fmt::printf("STATE 0 INCREMENT\n");
            
            if (c < RuneSelf) {
                // ASCII fast path
                continue;
            }
              
            uint8 x = first[c];
            if (x == xx) {
                // invalid
                continue;
            }
            
            state = 1;
            sz = int(x & 7); // size is 2, 3, or 4
            accept = accept_ranges[x>>4];
            // fmt::printf("lo %v; hi %v\n", accept.lo, accept.hi);
            continue;
          }
        
          case 1:
              if (c < accept.lo || c > accept.hi) {
                  //invalid
                  state = 0;
                  goto restart;
              }
              
              if (sz == 2) {
                  state = 0;
                  continue;
              }
              
              state = 2;
            //   fmt::printf("STATE 1 3\n");
              continue;
              
          case 2:
        //   fmt::printf("STATE 2\n");
            if (c < locb || c > hicb) {
                runecount += 1;
                state = 0;
                goto restart;
            }
            
            if (sz == 3) {
                state = 0;
                continue;
            }
            
            state = 3;
            continue;
            
          case 3:
        //   fmt::printf("STATE 3\n");
            if (c < locb || c > hicb) {
                runecount += 2;
                state = 0;
                goto restart;
            }
            state = 0;
            continue;
        }
    }
    
    return runecount;
}

size RuneDecoder::decode(str s, size limit, io::Writer &w, error err) {
    // fmt::printf("decode with limit %v; state %v\n", limit, state);
    size i = -1;
    size runecount = 0;
    for (byte c : s) {
        i++;

        restart:
        if (runecount >= limit) {
            size avail = i;
            if (runecount > limit) {
                avail -= runecount - limit;
            }
            w.write(s[0,avail], err);
            return runecount;
        }

        switch (state) {

            case 0: {
                // 0 bytes read, cnt == 1 on entry
                if (c < RuneSelf) {
                    // ASCII fast path
                    runecount++;
                    continue;
                }
                    
                uint8 x = first[c];
                if (x == xx) {
                    // invalid
                    runecount++;
                    continue;
                }
                
                state = 1;
                sz = int(x & 7); // size is 2, 3, or 4
                accept = accept_ranges[x >> 4];
                continue;
            }
        
            case 1:
                // 1 byte ready, cnt == 1 on entry
                if (c < accept.lo || c > accept.hi) {
                    //invalid
                    state = 0;
                    runecount++;
                    if (i == 0) {  
                        w.write_byte(buffer[0], err);
                    }
                    goto restart;
                }
                
                if (sz == 2) {
                    state = 0;
                    runecount++;
                    if (i == 0) {  
                        w.write_byte(buffer[0], err);
                    }
                    continue;
                }
                
                state = 2;
                continue;
                
            case 2:
                // 2 byte read, cnt == 2 on entry
                if (c < locb || c > hicb) {
                    state = 0;
                    runecount += 2;
                    if (i == 0) {
                        w.write(buffer[0, math::min(size(2), limit)], err);
                    }
                    goto restart;
                }
                
                if (sz == 3) {
                    state = 0;
                    runecount++;
                    if (i == 0) {
                        // fmt::printf("dumping buffer %v\n", buffer);
                        w.write(buffer[0,2], err);
                    }
                    continue;
                    // return 3;
                }
            
                state = 3;
                // fmt::printf("ENTER STATE 3\n");
                continue;
            
            case 3:
                // 3 bytes read, cnt == 3 on entry
                state = 0;
                if (c < locb || c > hicb) {
                    runecount += 3;
                    if (i == 0) {
                        w.write(buffer[0, math::min(size(2), limit)], err);
                    }
                    goto restart;
                }
                if (i == 0) {
                    w.write(buffer[0,3], err);
                }
                runecount++;
                continue;
        }
    }

    size end = i+1;
    if (state == 1) {
        if (i >= 0) buffer[0] = s[len(s)-1];
        // fmt::printf("SAVED 1 to buffer\n");

        end = std::max(size(0), end-1);
    } else if (state == 2) {
        if (i >= 0) buffer[1] = s[len(s)-1];
        if (i >= 1) buffer[0] = s[len(s)-2];
        // fmt::printf("SAVED 2 to buffer\n");

        end = std::max(size(0), end-2);
    } else if (state == 3) {
        if (i >= 0) buffer[2] = s[len(s)-1];
        if (i >= 1) buffer[1] = s[len(s)-2];
        if (i >= 2) buffer[0] = s[len(s)-3];

        end = std::max(size(0), end-3);
    }

    // fmt::printf("END end %v\n", end);
    w.write(s[0,end], err);

    return runecount;
}

rune RuneDecoder::decode_rune(str s, bool eof, int &bytes_consumed, bool &ok, bool &is_valid) {
    int i = 0;
    // size runecount = 0;

    for (byte c : s) {
        i++;

        switch (state) {

            case 0: {
                // 0 bytes read, cnt == 1 on entry
                if (c < RuneSelf) {
                    // ASCII fast path
                    // runecount++;
                    return bytes_consumed=1, ok=true, is_valid=true, rune(c);
                }
                    
                uint8 x = first[c];
                if (x == xx) {
                    // invalid
                    // runecount++;
                    return bytes_consumed=1, ok=true, is_valid = false, rune(c);
                }
                
                state = 1;
                sz = int(x & 7); // size is 2, 3, or 4
                accept = accept_ranges[x >> 4];
                c0 = c;
                continue;
            }
        
            case 1:
                // 1 byte ready, cnt == 1 on entry
                if (c < accept.lo || c > accept.hi) {
                    //invalid
                    state = 0;
                    // runecount++;
                    return bytes_consumed=0, ok=true, is_valid=false, rune(c0);
                }
                
                c1 = c;
                if (sz == 2) {
                    state = 0;
                    // runecount++;
                        
                    rune r = ((c0 & Mask2) << 6) | (c1 & MaskX);
                    return bytes_consumed=i, ok=true, is_valid=true, r;
                }
                
                state = 2;
                continue;
                
            case 2:
                // 2 byte read, cnt == 2 on entry
                if (c < locb || c > hicb) {
                    // runecount += 2;
                    
                    state = 4;
                    return bytes_consumed=0, ok=true, is_valid=false, rune(c0);
                    
                }

                c2 = c;
                if (sz == 3) {
                    state = 0;
                    // runecount++;
                    rune r = r = ((c0 & Mask3) << 12) | ((c1 & MaskX) << 6) | (c2 & MaskX);
                    return bytes_consumed=i, ok=true, is_valid=true, r;
                }
            
                state = 3;
                continue;
            
            case 3: {
                // 3 bytes read, cnt == 3 on entry
                state = 0;
                if (c < locb || c > hicb) {
                    runecount += 3;
                    
                    state = 5;
                    return bytes_consumed=0, ok=true, is_valid=false, rune(c0);
                }

                byte c3 = c;
                // runecount++;
                rune r = ((c0 & Mask4) << 18) | ((c1 & MaskX) << 12) | ((c2 & MaskX) << 6) | (c3 & MaskX);
                return bytes_consumed=i, ok=true, is_valid=true, r;
            }

            case 4:
                state = 0;
                return bytes_consumed=0, ok=true, is_valid=false, rune(c1);

            case 5:
                state = 6;
                return bytes_consumed=0, ok=true, is_valid=false, rune(c1);

            case 6:
                state = 0;
                return bytes_consumed=0, ok=true, is_valid=false, rune(c2);

        }
    }

    if (eof) {
        switch (state) {
            case 0:
                return bytes_consumed=0, ok=false, is_valid=0, rune(0);

            case 1:
                state = 0;
                return bytes_consumed=0, ok=true, is_valid=0, rune(c0);

            case 2:
                state = 4;
                return bytes_consumed=0, ok=true, is_valid=false, rune(c0);

            case 3:
                state = 5;
                return bytes_consumed=0, ok=true, is_valid=false, rune(c0);

            // 2 cont
            case 4:
                state = 0;
                return bytes_consumed=0, ok=true, is_valid=false, rune(c1);

            // 3 cont 1
            case 5:
                state = 6;
                return bytes_consumed=0, ok=true, is_valid=false, rune(c1);

            // 3 cont 2
            case 6:
                state = 0;
                return bytes_consumed=0, ok=true, is_valid=false, rune(c2);
        }       
    }

    return bytes_consumed=i, ok=false, is_valid=false, rune(0);
}

void RuneDecoder::decode_eof(size limit, io::Writer &w, error err) {
    // fmt::printf("decode eof with limit %v; state %v\n", limit, state);
    if (limit == 0) {
        return;
    }

    switch (state) {
        case 1:
            w.write(buffer[0,1], err);
            break;

        case 2:
            w.write(buffer[0, math::min(size(2), limit)], err);
            break;

        case 3:
            w.write(buffer[0, math::min(size(3), limit)], err);
            break;
    }
}

size RuneDecoder::eof() {
    size count = runecount;

    switch (state) {
        case 2:
            count++;
            break;

        case 3:
            count += 2;
            break;
    }

    return count;
}

size utf8::rune_count(str s) {
    RuneDecoder state;
    state.count(s);
    return state.eof();
}

size utf8::rune_count(io::WriterTo const& writable) {
    RuneCountingForwarder counter;
    
    writable.write_to(counter, error::ignore);
    
    return counter.count();
}

size RuneCountingForwarder::direct_write(str s, error err) {
    state.count(s);
    
    if (!out) {
        return len(s);
    }
    
    return out->direct_write(s, err);
}

size RuneCountingForwarder::count() {
    return state.eof();
}

bool utf8::valid_rune(rune r) {
    if (0 <= r && r < SurrogateMin) {
		return true;
    }
	if (SurrogateMax < r && r <= MaxRune) {
		return true;
    }
    return false;
}

int utf8::rune_len(rune r) {
    if (r < 0) {
		return -1;
    }
	if (r <= Rune1Max) {
		return 1;
    }
    if (r <= Rune2Max) {
		return 2;
	} 
    if (SurrogateMin <= r && r <= SurrogateMax) {
		return -1;
    }
    if (r <= Rune3Max) {
		return 3;
    }
	if (r <= MaxRune) {
		return 4;
	}
	return -1;
}

RuneTruncater utf8::rune_truncate(io::WriterTo const &w, size n) {
    return RuneTruncater(w, n);
}

void RuneTruncater::write_to(io::Writer &w, error err) const {
    struct RuneTruncaterWritable : io::Writer {
        io::Writer &dest;
        size max_count = 0;
        size count = 0;
        RuneDecoder state;
    
        RuneTruncaterWritable(io::Writer &dest, size max_count) : dest(dest), max_count(max_count) {}
    
        size direct_write(str data, error err) override {
            count += state.decode(data, max_count - count, dest, err);

            return len(data);
        }
    } rtw(w, this->max_count);

    this->w->write_to(rtw, err);
    rtw.state.decode_eof(rtw.max_count - rtw.count, w, err);
}
