#include "utf8.h"

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

size RuneCounterState::count(str s) {
    size runecount = 0;
    
    for (byte c : s) {
        
        switch (state) {
          case 0: {
            runecount++;
            
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
            accept = accept_ranges[x >> 4];
            continue;
          }
        
          case 1:
              if (c < accept.lo || c > accept.hi) {
                  //invalid
                  runecount++;
                  state = 0;
                  continue;
              }
              
              if (sz == 2) {
                  state = 0;
                  continue;
              }
              
              state = 2;
              continue;
              
          case 2:
            if (c < locb || c > hicb) {
                runecount++;
                state = 0;
                continue;;
            }
            
            if (sz == 3) {
                state = 0;
                continue;
            }
            
            state = 3;
            continue;
            
          case 3:
            if (c < locb || c > hicb) {
                runecount++;
                state = 0;
                continue;
            }
            state = 0;
        }
    }
    
    return runecount;
}

size utf8::count(str s) {
    RuneCounterState state;
    return state.count(s);
}

size utf8::count(io::WriterTo const& writable) {
    RuneCounter counter;
    
    writable.write_to(counter, error::ignore);
    
    return counter.runecount;
}

size RuneCounter::direct_write(str s, error err) {
    runecount += state.count(s);
    
    if (!out) {
        return len(s);
    }
    
    return out->direct_write(s, err);
}
