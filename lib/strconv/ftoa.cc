#include "ftoa.h"

#include "deps/fmt/include/fmt/base.h"
#include "deps/fmt/include/fmt/format.h"
#include "lib/io/io.h"
#include "lib/strconv/itoa.h"
#include "lib/strconv/quote.h"

#include <cctype>
#include <variant>

using namespace lib;
using namespace lib::strconv;
using namespace lib::strconv::detail;

struct FloatInfo {
	uint mantbits;
	uint expbits;
	int bias;
} ;

struct OutputIt {
    io::Writer *w;
    error *err;

    constexpr OutputIt(io::Writer *w, error *err) : w(w), err(err) {}

    constexpr OutputIt& operator=(char c) {
        w->write(c, *err);
        return *this;
      }

    constexpr OutputIt& operator*() { return *this; }
    constexpr OutputIt& operator++() { return *this; }
    constexpr OutputIt& operator++(int) { return *this; }
} ;

const FloatInfo float32_info = {
	.mantbits = 23,
	.expbits = 8,
	.bias = -127
} ;

const FloatInfo float64_info = {
	.mantbits = 52,
	.expbits = 11,
	.bias = -1023
} ;


// %x: -0x1.yyyyyyyyp±ddd or -0x0p+0. (y is hex digit, d is decimal digit)
static void fmt_x(io::Writer &out, int prec, byte fmt, bool neg, uint64 mant, int exp, int flags, FloatInfo const &flt, error err) {
	if (mant == 0) {
		exp = 0;
	}

	// Shift digits so leading 1 (if any) is at bit 1<<60.
	mant <<= 60 - flt.mantbits;
	while (mant != 0 && (mant & (uint64(1)<<60)) == 0) {
		mant <<= 1;
		exp--;
	}

	// Round if requested.
	if (prec >= 0 && prec < 15) {
		uint shift = uint(prec * 4);
		uint64 extra = (mant << shift) & (uint64(1)<<(60 - 1));
		mant >>= 60 - shift;
		if ((extra|(mant&1)) > uint64(1)<<59) {
			mant++;
		}
		mant <<= 60 - shift;
		if ((mant&(uint64(1)<<61)) != 0) {
			// Wrapped around.
			mant >>= 1;
			exp++;
		}
	}

	const char *hex = lowerhex;
	if (fmt == 'X') {
		hex = upperhex;
	}

	// sign, 0x, leading digit
	if (neg) {
		out.write('-', err);
	} else if (flags & FlagPlus) {
		out.write('+', err);
	} else if (flags & FlagSpace) {
		out.write(' ', err);
	}
	out.write('0', err);
	out.write('x', err);
	out.write('0'+byte((mant>>60)&1), err);

	// .fraction
	mant <<= 4; // remove leading 0 or 1
	if (prec < 0 && mant != 0) {
		out.write('.', err);
		while (mant != 0) {
			out.write(hex[(mant>>60)&15], err);
			mant <<= 4;
		}
	} else if (prec > 0) {
		out.write('.', err);
		for (int i = 0; i < prec; i++) {
			out.write(hex[(mant>>60)&15], err);
			mant <<= 4;
		}
	} else if (flags & FlagSharp) {
		out.write('.', err);
	}

	// p±
	byte ch = 'p';
	// if (fmt == 'X') {
	// 	ch = 'P';
	// }
	out.write(ch, err);
	if (exp < 0) {
		ch = '-';
		exp = -exp;
	} else {
		ch = '+';
	}
	out.write(ch, err);

	// dd or ddd or dddd
	if (exp < 100) {
		out.write(byte(exp/10)+'0', err); 
		out.write(byte(exp%10)+'0', err);
	} else if (exp < 1000) {
		out.write(byte(exp/100)+'0', err);
		out.write(byte((exp/10)%10)+'0', err);
		out.write(byte(exp%10)+'0', err);
	} else  {
		out.write(byte(exp/1000)+'0', err);
		out.write(byte(exp/100)%10+'0', err);
		out.write(byte((exp/10)%10)+'0', err);
		out.write(byte(exp%10)+'0', err);
	}
}

// %b: -ddddddddp±ddd
static void fmt_b(io::Writer &out, bool neg, uint64 mant, int exp, FloatInfo const &flt, error err) {
	// sign
	if (neg) {
		out.write('-', err);
	}

	// mantissa
	format_bits(out, mant, 10, false, err);

	// p
	out.write('p', err);
	
	// ±exponent
	exp -= int(flt.mantbits);
	if (exp >= 0) {
		out.write('+', err);
	}
	format_bits(out, exp, 10, exp < 0, err);
}

static void pad(io::Writer &out, io::Buf &buffer, int flags, int width, error err) {
	size left_pad = 0;
	size right_pad = 0;
	size length = buffer.length();
	
	if (length < width) {
		size pad = width - length;
		if (flags & FlagMinus) {
			right_pad = pad;
		} else {
			left_pad = pad;
		}

	}

	//::printf("PAD width %d; length %ld; left_pad %ld; right_pad %ld\n", width, buffer.length(), left_pad, right_pad);

	if (left_pad > 0) {
		out.write_repeated(' ', left_pad, err);
	}
	
	out.write(buffer.bytes(), err);

	if (right_pad > 0) {
		out.write_repeated(' ', right_pad, err);
	}
}


// template <typename T>
// requires ::fmt::detail::is_fast_float<T>::value
static void ftoa(io::Writer &out, std::variant<float32,float64> val, char fmt, int prec, int flags, int width, error err) {
	// lib::fmt::printf("ftoa2 impl\n");
	OutputIt out_it(&out, &err);

	uint64 bits;
	FloatInfo const *flt;

	if (val.index() == 0) {
		flt = &float32_info;
		bits = std::bit_cast<uint32>(std::get<float32>(val));
	} else /*if constexpr (sizeof(val) == 8) */ {
		flt = &float64_info;
		bits = std::bit_cast<uint64>(std::get<float64>(val));
	} 

	bool neg = bits>>(flt->expbits+flt->mantbits) != 0;
	int exp = int(bits>>flt->mantbits) & ((1<<flt->expbits) - 1);
	uint64 mant = bits & ((uint64(1)<<flt->mantbits) - 1);

	::fmt::memory_buffer buffer;
	io::Buf bufrw(buf(buffer.data(), buffer.capacity()));

	if (exp == (1<<flt->expbits) - 1) {
		// Inf, NaN
		str s;
		
		if (mant != 0) {
			if (flags & FlagPlus) {
				bufrw.write('+', err);
			} else if (flags & FlagSpace) {
				bufrw.write(' ', err);
			}
			s = "NaN";
		} else if (neg) {
			s = "-Inf";
		} else {
			bufrw.write(flags & FlagSpace ? ' ' : '+', err);
			s = "Inf";
		}
		
		bufrw.write(s, err);
		pad(out, bufrw, flags, width, err);
		return;
	} 
	
	if (exp == 0) {
		// denormalized
		exp++;
	} else {
		// add implicit top bit
		mant |= uint64(1) << flt->mantbits;
	}
	exp += flt->bias;

	// Pick off easy binary, hex formats.
	if (fmt == 'b') {
		fmt_b(bufrw, neg, mant, exp, *flt, err);
		pad(out, bufrw, flags, width, err);
		return;
	}
	if (fmt == 'x' || fmt == 'X') {
		// printf("PREC %d\n", prec);
		fmt_x(bufrw, prec, fmt, neg, mant, exp, flags, *flt, err);
		pad(out, bufrw, flags, width, err);
		return;
	}

	
    ::fmt::format_specs specs {};
	specs.precision = prec;

	if (isupper(fmt)) {
		specs.set_upper();
	}

	switch (tolower(fmt)) {
	case 'e':
		specs.set_type(::fmt::presentation_type::exp);
		break;

	case 'f':
		specs.set_type(::fmt::presentation_type::fixed);
		break;

	case 'g':
		specs.set_type(::fmt::presentation_type::general);
		break;

	default:
		// unknown format
		out.write('%', err);
		out.write(fmt, err);
		return;
	}

	::fmt::sign sign = ::fmt::sign::none;

	if (flags & FlagPlus) {
		sign = neg ? ::fmt::sign::minus : ::fmt::sign::plus;
	} else if (flags & FlagSpace) {
		sign = ::fmt::sign::space;
	} else if (neg) {
		sign = ::fmt::sign::minus;
	}

	bool want_decimal = flags & FlagSharp;
	// ::printf("fmt %c\n", fmt);

	// printf("SET WIDTH to %d\n", width);
	if (width > 0) {
		specs.width = width;

		if (flags & FlagMinus) {
			specs.set_align(::fmt::align::left);
			specs.set_fill(' ');
		} else if (flags & FlagZero) {
			specs.set_align(::fmt::align::numeric);
			specs.set_fill('0');
		} else {
			specs.set_align(::fmt::align::right);
			specs.set_fill(' ');
		}
	}
		

	bool shortest = prec < 0;
	if (shortest) {
		if (want_decimal) {
			// default precision %#g is 6
			specs.set_alt();
			specs.precision = 6;
		}

		if (val.index() == 0) {
			float32 v = std::get<float32>(val);
			// auto s = ::fmt::detail::signbit(v) ? ::fmt::sign::minus : ::fmt::sign::none;
			using floaty = ::fmt::conditional_t<sizeof(float32) >= sizeof(double), double, float>;
			auto dec = ::fmt::detail::dragonbox::to_decimal(static_cast<floaty>(v));
			::fmt::detail::write_float<char>(out_it, dec, specs, sign, 6, {});
		} else {
			float64 v = std::get<float64>(val);
			// auto s = ::fmt::detail::signbit(v) ? ::fmt::sign::minus : ::fmt::sign::none;
			using floaty = ::fmt::conditional_t<sizeof(float64) >= sizeof(double), double, float>;
			auto dec = ::fmt::detail::dragonbox::to_decimal(static_cast<floaty>(v));
			::fmt::detail::write_float<char>(out_it, dec, specs, sign, 6, {});
		}
  		
		return;
	}

	if (want_decimal) {
		// alternate format: add leading 0b for binary (%#b), 0 for octal (%#o),
		// 0x or 0X for hex (%#x or %#X); suppress 0x for %p (%#p);
		// for %q, print a raw (backquoted) string if [strconv.CanBackquote]
		// returns true;
		// always print a decimal point for %e, %E, %f, %F, %g and %G;
		// do not remove trailing zeros for %g and %G;
		specs.set_alt();
	}

	specs.set_sign(sign);
	// if (flags & FlagSpace) {
	// 	specs.set_sign(::fmt::sign::space);
	// }
	// if (flags & FlagPlus) {
	// 	specs.set_sign(::fmt::sign::plus);
	// }

	if (val.index() == 0) {
    	::fmt::detail::write_float<char>(out_it, std::get<float32>(val), specs, {});
	} else {
		::fmt::detail::write_float<char>(out_it, std::get<float64>(val), specs, {});
	}
}

Float64Formatter strconv::format_float(float64 f, char fmt, int prec, int flags, int width) { 
    return Float64Formatter(f, fmt, prec, flags, width);
};

Float32Formatter strconv::format_float(float32 f, char fmt, int prec, int flags, int width) { 
    return Float32Formatter(f, fmt, prec, flags, width);
};


void Float64Formatter::write_to(io::Writer &out, error err) const {
    ftoa(out, this->f, this->fmt, this->prec, this->flags, this->width, err);
}

void Float32Formatter::write_to(io::Writer &out, error err) const {
    ftoa(out, this->f, this->fmt, this->prec, this->flags, this->width, err);
}

