import lib.types;
import lib.unicode.graphic;
import lib.unicode;
import lib.unicode.tables;
import lib.unicode.letter;

using namespace lib;
using namespace lib::unicode;

bool unicode::is_space(rune r) {
    if (r < MaxLatin1) {
        switch (r) {
            case '\t': case '\n': case '\v': case '\f': case '\r':
            case ' ': case 0x85: case 0xA0:
                return true;
        }
        return false;
    }
    return is_excluding_latin(White_Space, r);
}

