#include "fmt.h"
#include <limits>
#include <type_traits>
#include "lib/io/io_stream.h"
#include "lib/strconv/ftoa.h"
#include "lib/types.h"
#include "lib/utf8/decode.h"
#include "lib/utf8.h"
#include "lib/strconv/quote.h"
#include "lib/strings/strings.h"
#include "lib/utf8/encode.h"
#include "lib/utf8/utf8.h"

using namespace lib;
using namespace fmt;

static const str percent_bang_string = "%!";
static const str ldigits = "0123456789abcdefx";
static const str udigits = "0123456789ABCDEFx";


State::State(io::Writer &out, str format, error err) :
    Fmt(out, err),
    begin(format.begin()),
    end(format.end()) {}


// https://github.com/mpaland/printf

bool State::advance() {
    const char *itr = begin;
    char c;
    if (this->verb == '*') {
        goto advance_fmtstr;
    }

    for (; itr != end; itr++) {
        c = *itr;

        if (c == '%') {
            out.write(str(begin, itr - begin), err);

            // reset
            wid = 0;
            prec  = 0;
            base  = 10;
            flags = 0;

            itr++;
        advance_fmtstr:
            rune verb;

            for (;;itr++) {
                if (itr == end) {
                    return false;
                }
                char c = *itr;
                verb = c;
                
                switch (c) {
                case '#':
                    sharp = true;
                    continue;

                case '0':
                    zero = true;
                    continue;

                case '+':
                    plus = true;
                    continue;

                case '-':
                    minus = true;
                    continue;
                
                case ' ':
                    space = true;
                    continue;

                case '.':
                    prec_present = true;
                    itr++;
                    while (itr != end && '0' <= *itr && *itr <= '9') {
                        prec = prec*10 + (*itr - '0');
                        itr++;
                    }
                    itr--;
                    continue;

                case 'b': /* fallthrough*/
                case 'd': /* fallthrough*/
                case 'g': /* fallthrough*/
                case 'G': /* fallthrough*/
                case 'f': /* fallthrough*/
                case 'F': 
                    break;

                case 'x':
                    base = 16;
                    break;

                case 'v':
                    this->sharp_v = this->sharp;
                    this->sharp = false;
                    this->plus_v = this->plus;
                    this->plus = false;
                    break;

                case 'q':
                    break;

                case '*':
                    break;

                default:
                    if ('0' <= c && c <= '9') {
                        while (itr != end && '0' <= *itr && *itr <= '9') {
                            wid = wid*10 + (*itr - '0');
                            itr++;
                        }
                        itr--;
                        wid_present = true;
                        continue;
                    }

                    if (verb > utf8::RuneSelf) {
                        int nbytes;
                        verb = utf8::decode_rune(str(itr, end-itr), nbytes);
                        itr += nbytes-1;
                    }

                    break;

                }

                break;
            }

            this->verb = verb;
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

void Fmt::handle_star(int n) {
    Fmt &f = *this;

    // ::printf("HANDLE STAR %d\n", f.prec_present);
    if (f.prec_present) {
        if (n < 0 || n > 1'000'000) {
            f.fmt_bad_int_arg();
            return;
        }

        f.prec = n;
        return;
    }
    
    // set width
    if (math::abs(n) > 1'000'000) {
        f.fmt_bad_int_arg();
        return;
    }
    if (n < 0) {
        n = -n;
        f.minus = true;
    }
    f.wid = n;
    f.wid_present = true;
}

template <typename T>
void Fmt::write_integer(T n, bool is_signed) {
    Fmt &f = *this;
    // const str digits = upcase ? udigits : ldigits;

    switch (f.verb) {
    case 'v':
        if (f.sharp_v && !is_signed) {
            f.fmt_integer(n, is_signed, 16, char(f.verb), ldigits, true);
        } else {
            f.fmt_integer(n, is_signed, 10, char(f.verb), ldigits, f.sharp);
        }
        break;
    case 'd':
        f.fmt_integer(n, is_signed, 10, char(f.verb), ldigits, f.sharp);
        break;
    case 'b':
        f.fmt_integer(n, is_signed, 2, char(f.verb), ldigits, f.sharp);
        break;
    case 'O': /* fallthrough*/
    case 'o':
        f.fmt_integer(n, is_signed, 8, char(f.verb), ldigits, f.sharp);
        break;
    case 'x':
        f.fmt_integer(n, is_signed, 16, char(f.verb), ldigits, f.sharp);
        break;
    case 'X':
        f.fmt_integer(n, is_signed, 16, char(f.verb), udigits, f.sharp);
        break;
    case 'c':
        f.fmt_c(n);
        break;
    case 'q':
        f.fmt_qc(n);
        break;
    case 'U':
        f.fmt_unicode(n);
        break;
    case '*':
        if constexpr (sizeof(T) > sizeof(int)) {
            if (
                (is_signed && (std::make_signed_t<T>(n)) > std::numeric_limits<int>::max()) ||
                (!is_signed && (n > std::numeric_limits<int>::max()))
            ) {
                f.fmt_bad_int_arg();
                return;
            }
        }
        f.handle_star(int(n));
        break;
    default:
        f.bad_verb(verb);
        break;
    }
}

template <typename T>
void Fmt::write_char(T n, bool is_signed) {
    Fmt &f = *this;
    rune verb = f.verb;
    if (f.verb == 'v') {
        f.verb = 'c';
    }
    write_integer(n, is_signed);
    f.verb = verb;
}

template <typename T>
void Fmt::fmt_integer(T n, bool is_signed, int base, char verb, str digits, bool sharp) {
    Fmt &f = *this;

    char buf[256];
    
    char sep = comma ? ',' : apos ? '\'' : 0;
    int nsep = (base == 16 || base == 2) ? 5 : 4;

    //bool negative = is_signed && n >= T(-1);
    bool negative = is_signed && ((std::make_signed_t<T>) n) < 0;
    if (negative) {
        n = -n;
    }

    size prec = f.prec;
    if (f.prec_present) {
        // Precision of 0 and value of 0 means "print nothing" but padding.
		if (prec == 0 && n == 0) {
			bool old_zero = f.zero;
			f.zero = false;
			f.write_padding(f.wid);
			f.zero = old_zero;
			return;
		} else if (f.zero && !f.minus && f.wid_present) { // Zero padding is allowed only to the left.
            // prec = f.wid;
            // if (negative || f.plus || f.space) {
            //     prec--; // leave room for sign
            // }
        }
    }

    char padchar = ' ';
    if (f.zero && !(f.prec_present && f.wid_present)) {
        padchar = '0';
    }

    // Because printing is easier right-to-left: format u into buf, ending at buf[i].
    size i = sizeof buf;
    switch (base) {
    case 10:
        while (n >= 10) {
            i--;
            T next = n / 10;
            buf[i] = byte('0' + n - next*10);
            n = next;
        }
        break;
    case 16:
        while (n >= 16) {
            i--;
            buf[i] = digits[n&0xF];
            n >>= 4;
        }
        break;
    case 8:
        while (n >= 8) {
            i--;
            buf[i] = byte('0' + (n&7));
            n >>= 3;
        }
        break;
    case 2:
        while (n >= 2) {
            i--;
            buf[i] = byte('0' + (n&1));
            n >>= 1;
        }
        break;
    default:
        panic("fmt: unkown base; can't happen");
    }
    i--;
    buf[i] = digits[n];

    size ncount = sizeof(buf) - i;
    // size ncount = sizeof(buf) - i;

    // while (i > 0 && prec > sizeof(buf)-i) {
    //     i--;
    //     buf[i] = '0';
    // }

    // size ncount = sizeof(buf) - i;
    // if (n == 0) {
    //     buf[--i] = '0';
    //     ncount++;
    // } else {
    //     if (base == 10) {
    //         while (n >= 10) {
    //             i--;
    //             T next = n / 10;
    //             buf[i] = byte('0' + n - next*10);
    //             n = next;
    //             ncount++;
    //         }
    //         i--;
    //         ncount++;
    //         buf[i] = digits[n];
    //     } else {
    //         do {
    //             ncount++;
    //             if (sep && (ncount % nsep) == 0) {
    //                 buf[--i] = sep;
    //                 ncount++;
    //             }
    //             buf[--i] = digits[n % base];
    //             n /= base;
    //         } while (n > 0);
    //     }
    // }

    size pad_prec = prec > ncount ? prec - ncount : 0;
    // pad_prec = 0;
    if (base == 8 && verb != 'O') {
        pad_prec--;
    }
    size sharp_extra = !sharp ? 0
                            : (base == 16 || base == 2) ? 2
                                : base == 8 ? verb == 'O' ? 2 : 1
                                    : 0;
    ncount += pad_prec + sharp_extra;
    if (negative||plus||space) ncount++;
    size pad_width = wid > ncount ? wid-ncount : 0;

    bool left_pad = pad_width > 0 && padchar == ' ' && !f.minus;
    if (pad_width > 0 && padchar == '0') {
        pad_prec += pad_width;
    }
    bool right_pad = !left_pad && f.minus;

    if (left_pad) {
        out.write_repeated(padchar, pad_width, err);
    }
    if (negative)
        out.write('-', error::ignore);
    else if (plus)
        out.write('+', error::ignore);
    else if (space)
        out.write(' ', error::ignore);
    if (sharp) {
        if (base == 8)       out.write('0', err);
        else if (base == 2)  out.write("0b", err);
        else if (base == 16) out.write("0x", err);
    }
    if (verb == 'O') {
        out.write("0o", err);
    }

    while (i > 0 && pad_prec > 0) {
        i--;
        pad_prec--;
        buf[i] = '0';
    }
    if (pad_prec > 0) {
        out.write_repeated('0', pad_prec, err);
    }

    out.write(str(buf+i, sizeof(buf)-i), err);
    if (right_pad) {
        out.write_repeated(padchar, pad_width, err);
    }
}

void Fmt::fmt_float(std::variant<float32, float64> v, rune verb, int prec) {
    Fmt &f = *this;

    // Explicit precision in format specifier overrules default precision.
	if (f.prec_present) {
		prec = f.prec;
	}

    int flags = 0;
    if (f.plus || f.plus_v) {
        flags |= strconv::FlagPlus;
    }
    if (f.minus) {
        flags |= strconv::FlagMinus;
    }
    if (f.zero) {
        flags |= strconv::FlagZero;
    }
    if (f.space) {
        flags |= strconv::FlagSpace;
    }
    if (f.sharp) {
        flags |= strconv::FlagSharp;
    }

    if (v.index() == 0) {
        strconv::format_float(std::get<float32>(v), char(verb), prec, flags, f.wid).write_to(f.out, f.err);
    } else {
        strconv::format_float(std::get<float64>(v), char(verb), prec, flags, f.wid).write_to(f.out, f.err);
    }
}

template <typename T>
void Fmt::fmt_c(T c) {
    Fmt &f = *this;
    // Explicitly check whether c exceeds utf8.MaxRune since the conversion
	// of a uint64 to a rune may lose precision that indicates an overflow.
	rune r = rune(c);
	if (c > utf8::MaxRune) {
		r = utf8::RuneError;
	}
	// buf := f.intbuf[:0]
    Array<byte, utf8::UTFMax> buf;
    f.write_padded(utf8::encode(buf, r));
}


template <typename T>
void Fmt::fmt_qc(T c) {
    Fmt &f = *this;
    rune r = rune(c);
	if (c > utf8::MaxRune) {
		r = utf8::RuneError;
	}

	if (f.plus) {
        f.write_padded(strconv::quote_rune_to_ascii(r));
	} else {
		f.write_padded(strconv::quote_rune(r));
	}
}

template <typename T>
void Fmt::fmt_unicode(T u) {
    Fmt &f = *this;

	// With default precision set the maximum needed buf length is 18
	// for formatting -1 with %#U ("U+FFFFFFFFFFFFFFFF") which fits
	// into the already allocated intbuf with a capacity of 68 bytes.
	int prec = 4;
	if (f.prec_present && f.prec > 4) {
		prec = f.prec;
		// Compute space needed for "U+" , number, " '", character, "'".
		// size width = 2 + prec + 2 + utf8::UTFMax + 1;
		// if (width > len(buf)) {
		// 	buf = make([]byte, width)
		// }
	}

	// Format into buf, ending at buf[i]. Formatting numbers is easier right-to-left.
    //char buf[256];
    Array<byte, 256> buf;
	size i = len(buf);

	// For %#U we want to add a space and a quoted character at the end of the buffer.
	if (f.sharp && u <= utf8::MaxRune && strconv::is_print(rune(u))) {
		i--;
		buf[i] = '\'';
		i -= utf8::rune_len(rune(u));
		utf8::encode_rune(buf+i, rune(u));
		i--;
		buf[i] = '\'';
		i--;
		buf[i] = ' ';
	}
	// Format the Unicode code point u as a hexadecimal number.
	while (u >= 16) {
		i--;
		buf[i] = udigits[u&0xF];
		prec--;
		u >>= 4;
	}
	i--;
	buf[i] = udigits[u];
	prec--;
	// Add zeros in front of the number until requested precision is reached.
	while (prec > 0) {
		i--;
		buf[i] = '0';
		prec--;
	}
	// Add a leading "U+".
	i--;
	buf[i] = '+';
	i--;
	buf[i] = 'U';

	bool old_zero = f.zero;
	f.zero = false;
	f.write_padded(buf+i);
	f.zero = old_zero;
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

void Fmt::write_padded(str s){
    Fmt &f = *this;
    if (f.wid == 0) {
        f.out.write(s, f.err);
        return;
    }

    size runecount = utf8::rune_count(s);
    if (runecount >= f.wid) {
        f.out.write(s, f.err);
        return;
    }

    bool left = !f.minus;
    size padamt = f.wid - runecount;

    if (left) {
        char padchar = f.zero ? '0' : ' ';
        f.out.write_repeated(padchar, padamt, f.err);
        f.out.write(s, f.err);
    } else {
        f.out.write(s, f.err);
        f.out.write_repeated(' ', padamt, f.err);
    }
}

void Fmt::write_padded(io::WriterTo const& writable) {
    Fmt &f = *this;
    if (f.wid == 0) {
        writable.write_to(f.out, f.err);
        return;
    }

    bool left = !f.minus;

    if (left) {
        size runecount = utf8::rune_count(writable);
        size padamt = f.wid - runecount;
        if (padamt > 0) {
            char padchar = f.zero ? '0' : ' ';
            f.out.write_repeated(padchar, padamt, f.err);
        }
        writable.write_to(f.out, f.err);
    } else {
        utf8::RuneCountingForwarder counter(f.out);
        writable.write_to(counter, f.err);
        size padamt = f.wid - counter.count();
        if (padamt > 0) {
            f.out.write_repeated(' ', padamt, f.err);
        }
    }
}

//template
//void Fmt::write_integer(unsigned int n, bool);

void Fmt::fmt_bad_int_arg() {
    Fmt &f = *this;

    if (f.prec_present) {
        write_string("%!(BADPREC)");
    } else {
        write_string("%!(BADWIDTH)");
    }
}

void Fmt::write(str v) {
    Fmt &f = *this;
    switch (verb) {
        case 'v':
            if (f.sharp_v) {
                f.fmt_q(v);
            } else {
                f.fmt_s(v);
            }
            break;
        case 's':
            f.fmt_s(v);
            break;
        case 'x':
            f.fmt_sx(v, ldigits);
            break;
        case 'X':
            f.fmt_sx(v, udigits);
            break;
        case 'q':
            f.fmt_q(v);
            break;
        case '*':
            f.fmt_bad_int_arg();
            break;
        default:
            f.bad_verb(verb);
            break;
            
    }

    // write_padded(*this, v);
}

void Fmt::fmt_q(str s) {
    Fmt &f = *this;
    s = f.truncate_string(s);
	if (f.sharp && strconv::can_backquote(s)) {
		f.write_padded(strings::cat("`", s, "`"));
		return;
	}
	if (f.plus) {
        f.write_padded(strconv::quote_to_ascii(s));
	} else {
        f.write_padded(strconv::quote(s));
	}
}

void Fmt::fmt_qw(io::WriterTo const &w) {
    Fmt &f = *this;
    io::WriterTo const *out = &w;
    utf8::RuneTruncater t;

    if (f.prec_present) {
        t = utf8::rune_truncate(w, f.prec);
        out = &t;
    }
    
    if (f.sharp && strconv::can_backquote(*out)) {
        f.write_padded(cat('`', *out, '`'));
        return;
    }

    if (f.plus) {
        f.write_padded(strconv::quote_to_ascii(*out));
	} else {
        f.write_padded(strconv::quote(*out));
	}
}

void Fmt::fmt_s(str s) {
    Fmt &f = *this;
    s = f.truncate_string(s);
	f.write_padded(s);
}

void Fmt::fmt_w(io::WriterTo const &w) {
    Fmt &f = *this;
    if (f.prec_present) {
        f.write_padded(utf8::rune_truncate(w, f.prec));
        return;
    }

    f.write_padded(w);
}

void Fmt::fmt_sx(str s, str digits) {
    Fmt &f = *this;
    size length = len(s);

	// Set length to not process more bytes than the precision demands.
	if (f.prec_present && f.prec < length) {
		length = f.prec;
	}

	// Compute width of the encoding taking into account the f.sharp and f.space flag.
	size width = 2 * length;
	if (width > 0) {
		if (f.space) {
			// Each element encoded by two hexadecimals will get a leading 0x or 0X.
			if (f.sharp) {
				width *= 2;
			}
			// Elements will be separated by a space.
			width += length - 1;
		} else if (f.sharp) {
			// Only a leading 0x or 0X will be added for the whole string.
			width += 2;
		}
	} else { // The byte slice or string that should be encoded is empty.
		if (f.wid_present) {
			f.write_padding(f.wid);
		}
		return;
	}
	// Handle padding to the left.
	if (f.wid_present && f.wid > width && !f.minus) {
		f.write_padding(f.wid - width);
	}
	
    // // Write the encoding directly into the output buffer.
	// // buf := *f.buf

	if (f.sharp) {
		// Add leading 0x or 0X.
        f.write_byte('0');
        f.write_byte(digits[16]);
	}
	byte c = 0;
	for (size i = 0; i < length; i++) {
		if (f.space && i > 0) {
			// Separate elements with a space.
            f.write_byte(' ');
			if (f.sharp) {
				// Add leading 0x or 0X for each element.
				f.write_byte('0');
                f.write_byte(digits[16]);
			}
		}
		
        c = s[i]; // Take a byte from the input string.
		
		// Encode each byte as two hexadecimal digits.
		f.write_byte(digits[c>>4]);
        f.write_byte(digits[c&0xF]);
	}

	// Handle padding to the right.
	if (f.wid_present && f.wid > width && f.minus) {
		f.write_padding(f.wid - width);
	}
}

void Fmt::fmt_wx(io::WriterTo const &w, str digits) {
    struct Writer : io::Writer {
        io::Writer *out = nil;
        size cnt = 0;
        bool prec_present;
        int prec = 0;
        bool sharp;
        bool space;
        str digits;
        
        size direct_write(str data, error err) override {

            for (byte b : data) {
                if (cnt == 0) {
                    if (sharp) {
                        // Add leading 0x or 0X.
                        out->write('0', err);
                        out->write(digits[16], err);
                    }
                }

                if (prec_present && cnt >= prec) {
                    break;
                }

                if (space && cnt > 0) {
                    // Separate elements with a space.
                    this->out->write(' ', err);
                    if (sharp) {
                        // Add leading 0x or 0X for each element.
                        this->out->write('0', err);
                        this->out->write(digits[16], err);
                    }
                }
		
                // Encode each byte as two hexadecimal digits.
                this->out->write(digits[b>>4], err);
                this->out->write(digits[b&0xF], err);

                cnt++;
            }

            return len(data);
        }
    };

    struct WriterToX : io::WriterTo {
        io::WriterTo const *w = nil;
        bool prec_present;
        int prec;
        bool sharp;
        bool space;
        str digits;
        
        void write_to(io::Writer &out, error err) const override {
            Writer writer;
            writer.out = &out;
            writer.prec_present = prec_present;
            writer.prec = prec;
            writer.sharp = sharp;
            writer.space = space;
            writer.digits = digits;
            
            this->w->write_to(writer, err);
        }

        
    } writer_to;

    writer_to.w = &w;
    writer_to.prec_present = prec_present;
    writer_to.prec = this->prec;
    writer_to.sharp = this->sharp;
    writer_to.space = this->space;
    writer_to.digits = digits;

    this->write_padded(writer_to);
}

void Fmt::write_padding(size n) {
    Fmt &f = *this;
    if (n <= 0) { // No padding bytes needed.
		return;
	}
    
    // Decide which byte the padding should be filled with.
	byte pad_byte = ' ';
	// Zero padding is allowed only to the left.
	if (f.zero && !f.minus) {
		pad_byte = '0';
	}

    f.write_repeated(pad_byte, n);
}


str Fmt::truncate_string(str s) {
    Fmt &f = *this;
    if (f.prec_present) {
		size n = f.prec;
		for (auto [i, _] : utf8::runes(s)) {
			n--;
			if (n < 0) { 
				return s[0,i];
			}
		}
	}
	return s;
}

// utf8::RuneTruncater Fmt::truncate_w(io::WriterTo const &w, size n) {
//     Fmt &f = *this;
//     if (f.prec_present) {
// 		size n = f.prec;
// 		for (auto [i, _] : utf8::runes(s)) {
// 			n--;
// 			if (n < 0) {
// 				return s[0,i];
// 			}
// 		}
// 	}
// 	return s;
// }


void Fmt::write_string(str s) {
    this->out.write(s, err);
}
void Fmt::write_byte(byte b) {
    this->out.write(b, err);
}

void Fmt::write_repeated(byte b, size count) {
    this->out.write_repeated(b, count, err);
}

void Fmt::write_rune(rune r) {
    Array<byte, utf8::UTFMax> b;
    str s = utf8::encode(b, r);
    this->write_string(s);
}

void Fmt::write_float(std::variant<float32,float64> v) {
    Fmt &f = *this;
    switch (f.verb) {
    case 'v':
        f.fmt_float(v, 'g', -1);
        break;

    case 'g':
    case 'G':
        // if (f.sharp) {
        //     // default precision %#g is 6
        //     f.fmt_float(v, verb, 6);
        //     break;
        // } 
        /* fallthrough */

    case 'b':
    case 'x':
    case 'X':
        f.fmt_float(v, verb, -1);
        break;

    case 'f':
    case 'e':
    case 'E':
        f.fmt_float(v, verb, 6);
        break;
    case 'F':
        f.fmt_float(v, 'f', 6);
        break;
    default:
        f.bad_verb(f.verb);
    }
}

void Fmt::bad_verb(rune) {
    Fmt &f = *this;

    // f.erroring = true;
	f.write_string(percent_bang_string);
	f.write_rune(verb);
	f.write_byte('(');
	
	// case p.arg != nil:
	// 	p.buf.writeString(reflect.TypeOf(p.arg).String())
	// 	p.buf.writeByte('=')
	// 	p.printArg(p.arg, 'v')
	// case p.value.IsValid():
	// 	p.buf.writeString(p.value.Type().String())
	// 	p.buf.writeByte('=')
	// 	p.printValue(p.value, 'v', 0)
	// default:
    // f.write_string(nil_angle_string);
    f.write_string("!");
	// }
	f.write_byte(')');
	// f.erroring = false;
}

void Fmt::write(io::WriterTo const& w) {
    Fmt &f = *this;
    switch (verb) {
        case 'v':
            if (f.sharp_v) {
                f.fmt_qw(w);
            } else {
                f.fmt_w(w);
            }
            break;
        case 's':
            f.fmt_w(w);
            break;
        case 'x':
            // f.write_string("<x unimplemented>");
            f.fmt_wx(w, ldigits);
            break;
        case 'X':
            //f.fmt_sx(v, udigits);
            f.fmt_wx(w, udigits);
            // f.write_string("<X unimplemented>");
            break;
        case 'q':
            f.fmt_qw(w);
            break;
        default:
            f.bad_verb(verb);
            break;
            
    }

    // write_padded(w);
}

void Fmt::write(unsigned char b) {
    if constexpr (sizeof(b) <= sizeof(uintptr_t)) {
        write_integer<uintptr_t>(b, false);
    } else {
        write_integer((unsigned short) b, false);
    }
}

void Fmt::write(char c) {
    if constexpr (sizeof(c) <= sizeof(uintptr_t)) {
        write_char<uintptr_t>(c, true);
    } else {
        write_char<uint32>(c, true);
    }
}

void Fmt::write(char32_t c) {
    if constexpr (sizeof(c) <= sizeof(uintptr_t)) {
        write_char<uintptr_t>(c, true);
    } else {
        write_char<uint32>(c, true);
    }
}

void Fmt::write(bool b) {
    if (b) {
        write_padded("true");
    } else {
        write_padded("false");
    }
}

void Fmt::write(short i) {
    if constexpr (sizeof(i) <= sizeof(uintptr_t)) {
        write_integer<uintptr_t>(i, true);
    } else {
        write_integer((unsigned short) i, true);
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
        write_integer<uintptr_t>(i, true);
    } else {
        write_integer((unsigned int) i, true);
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
        write_integer<uintptr_t>(i, true);
    } else {
        write_integer((unsigned long) i, true);
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
        write_integer<uintptr_t>(i, true);
    } else {
        write_integer((unsigned long long) i, true);
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
    write_float(f);
}

void Fmt::write(double d) {
    write_float(d);
}

void Fmt::write(String const &s) {
    write(str(s));
}

void Fmt::write(const char *p) {
    if (p == nil) {
        write(str("<nil>"));
        return;
    }
    write(str::from_c_str(p));
}

void Fmt::write(char *p) {
    write((const char *)p);
}

void Fmt::write(const wchar_t *p) {
    // const rune *rp = (const rune*) p;
    size length = wcslen(p);
    arr<const wchar_t> data(p, length);

    utf8::Encoder enc { data };
    write((io::WriterTo &) enc);
}

void Fmt::write(wchar_t *p) {
    write((const wchar_t *)p);
}

io::ReadResult BufferedWriter::direct_read(buf bytes, error err) {
    panic("unimplemented");
    return {};
}

size BufferedWriter::direct_write(str data, error err) {
    return this->out.direct_write(data, err);
}
