#include "fmt.h"
#include <limits>
#include "quote.h"
#include "lib/utf8.h"

using namespace lib;
using namespace fmt;

State::State(io::OStream &out, str format) :
    Fmt(out, error::ignore()),
    begin(format.begin()),
    end(format.end()) {}


// https://github.com/mpaland/printf

bool State::advance() {

    for (const char *itr = begin; itr != end; itr++) {
        char c = *itr;

        if (c == '%') {
            out.write(str(begin, itr - begin), err);

            // reset
            width = 0;
            prec  = 0;
            base  = 10;
            flags = 0;

            itr++;
            if (itr == end) {
                break;
            }

            switch (*itr) {
              case '%':
                begin = itr;
                continue;

              case 'x':
                base = 16;
                break;

              case 'q':
                quoted = true;
                break;

            }

            begin = itr+1;
            return true;
        }
    }

    return false;
}

void State::flush() {
    for (;;) {
        if (!advance()) {
            break;
        }

        out.write("<!>", err);
    }

    out.write(str(begin, end - begin), err);
}

static const str ldigits = "0123456789abcdefghijklmnopqrstuvwxyz";
static const str udigits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

template <typename T>
void Fmt::write_integer(T n, bool negative) {
    char buf[256];
    const str digits = upcase ? udigits : ldigits;
    char sep = comma ? ',' : apos ? '\'' : 0;
    int nsep = (base == 16 || base == 2) ? 5 : 4;

    //bool negative = is_signed && n >= T(-1);
    if (negative) {
        n = -n;
    }

    size i = sizeof buf;
    size ncount = 0;
    if (n == 0) {
        buf[--i] = '0';
        ncount++;
    } else {
        do {
            ncount++;
            if (sep && (ncount % nsep) == 0) {
                buf[--i] = sep;
                ncount++;
            }
            buf[--i] = digits[n % base];
            n /= base;
        } while (n > 0);
    }

    size pad_prec = prec > ncount ? prec - ncount : 0;
    size sharp_extra = !sharp ? 0
                            : (base == 16 || base == 2) ? 2
                                : base == 8 ? 1
                                    : 0;
    ncount += pad_prec + sharp_extra;
    if (negative||plus||space) ncount++;
    size pad_width = width > ncount ? width-ncount : 0;

    if (pad_width > 0) {
        char padchar = zero ? '0' : ' ';
        out.write_repeated(padchar, pad_width, err);
    }
    if (negative)
        out.write('-', error::ignore());
    else if (plus)
        out.write('+', error::ignore());
    else if (space)
        out.write(' ', error::ignore());
    if (sharp) {
        if (base == 8)       out.write('0', err);
        else if (base == 2)  out.write("0b", err);
        else if (base == 16) out.write("0x", err);
    }
    if (pad_prec > 0) {
        out.write_repeated('0', pad_prec, err);
    }
    out.write(str(buf+i, sizeof(buf)-i), err);
}

// template <typename T>
// void Fmt::write_float(T val) {

//     if (val != val) {
//         out.write("NaN", error::ignore());
//         return;
//     }

//     if (val < std::numeric_limits<T>::min()) {
//         out.write("-Inf", error::ignore());
//         return;
//     }

//     if (val > std::numeric_limits<T>::max()) {
//         out.write("+Inf", error::ignore());
//         return;
//     }



// }

static void write_padded(Fmt &f, str s){
    if (f.width == 0) {
        f.out.write(s, f.err);
        return;
    }

    size runecount = utf8::count(s);
    if (runecount >= f.width) {
        f.out.write(s, f.err);
        return;
    }

    bool left = !f.minus;
    size padamt = f.width - runecount;
    char padchar = f.zero ? '0' : ' ';

    if (left) {
        f.out.write_repeated(padchar, padamt, f.err);
        f.out.write(s, f.err);
    } else {
        f.out.write(s, f.err);
        f.out.write_repeated(padchar, padamt, f.err);
    }
}

static void write_padded(Fmt &f, io::WriterTo const& writable) {
    if (f.width == 0) {
        writable.write_to(f.out, f.err);
        return;
    }

    bool left = !f.minus;
    char padchar = f.zero ? '0' : ' ';

    if (left) {
        size runecount = utf8::count(writable);
        size padamt = f.width - runecount;
        if (padamt > 0) {
            f.out.write_repeated(padchar, padamt, f.err);
        }
        writable.write_to(f.out, f.err);
    } else {
        utf8::RuneCounter counter(f.out);
        writable.write_to(counter, f.err);
        size padamt = f.width - counter.runecount;
        if (padamt > 0) {
            f.out.write_repeated(padchar, padamt, f.err);
        }
    }
}

//template
//void Fmt::write_integer(unsigned int n, bool);

void Fmt::write(str s) {
    if (quoted) {
        write_padded(*this, quote(s));
        return;
    }

    write_padded(*this, s);
}

void Fmt::write(io::WriterTo const& w) {
//     if (quoted) {
//         write_padded(*this, quote(w));
//         return
//     }

    write_padded(*this, w);
}

void Fmt::write(byte b) {
    write((int) b);
}

void Fmt::write(char c) {
    write((byte) c);
}

void Fmt::write(bool b) {
    if (b) {
        write_padded(*this, "true");
    } else {
        write_padded(*this, "false");
    }
}

void Fmt::write(short i) {
    if constexpr (sizeof(i) <= sizeof(uintptr_t)) {
        write_integer<uintptr_t>(i, i < 0);
    } else {
        write_integer((unsigned short) i, i < 0);
    }
}

void Fmt::write(unsigned short i) {
    if constexpr (sizeof(i) <= sizeof(uintptr_t)) {
        write_integer<uintptr_t>(i, false);
    } else {
        write_integer(i, false);
    }
}

void Fmt::write(int i) {
    if constexpr (sizeof(i) <= sizeof(uintptr_t)) {
        write_integer<uintptr_t>(i, i < 0);
    } else {
        write_integer((unsigned int) i, i < 0);
    }
}

void Fmt::write(unsigned int i) {
    if constexpr (sizeof(i) <= sizeof(uintptr_t)) {
        write_integer<uintptr_t>(i, false);
    } else {
        write_integer(i, false);
    }
}

void Fmt::write(long i) {
    if constexpr (sizeof(i) <= sizeof(uintptr_t)) {
        write_integer<uintptr_t>(i, i < 0);
    } else {
        write_integer((unsigned long) i, i < 0);
    }
}

void Fmt::write(unsigned long i) {
    if constexpr (sizeof(i) <= sizeof(uintptr_t)) {
        write_integer<uintptr_t>(i, false);
    } else {
        write_integer(i, false);
    }
}

void Fmt::write(long long i) {
    if constexpr (sizeof(i) <= sizeof(uintptr_t)) {
        write_integer<uintptr_t>(i, i < 0);
    } else {
        write_integer((unsigned long long) i, i < 0);
    }
}

void Fmt::write(unsigned long long i) {
    if constexpr (sizeof(i) <= sizeof(uintptr_t)) {
        write_integer<uintptr_t>(i, false);
    } else {
        write_integer(i, false);
    }
}

void Fmt::write(float f) {
    write((double) f);
}

void Fmt::write(double d) {
    char buf[256];
    int count = snprintf(buf, sizeof buf, "%.3f", d);
    write(str(buf, count));
}

void Fmt::write(String const &s) {
    write(str(s));
}

void Fmt::write(const char *p) {
    write(str::from_c_str(p));
}

void Fmt::write(char *p) {
    write((const char *)p);
}

void Fmt::write(const wchar_t *p) {
    // const rune *rp = (const rune*) p;
    size length = wcslen(p);
    array<const wchar_t> data(p, length);

    utf8::Encoder enc { data };
    write((io::WriterTo &) enc);
}

void Fmt::write(wchar_t *p) {
    write((const wchar_t *)p);
}
