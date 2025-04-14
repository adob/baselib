#include "atoi.h"
#include "lib/fmt/fmt.h"

using namespace lib;
using namespace lib::strconv;

const uint64 MaxUint64 = -1;

static void syntax_error(error err, str fn, str s) {
    err(NumError(fn, s, ErrSyntax()));
}

static void range_error(error err, str fn, str s) {
    err(NumError(fn, s, ErrRange()));
}

static void base_error(error err, str fn, str s, int base) {
    err(NumError(fn, s, fmt::errorf("invalid base %v", base)));
}

static void bit_size_error(error err, str fn, str s, int bit_size) {
    err(NumError(fn, s, fmt::errorf("invalid bit size %v", bit_size)));
}

// lower(c) is a lower-case letter if and only if
// c is either that lower-case letter or the equivalent upper-case letter.
// Instead of writing c == 'x' || c == 'X' one can write lower(c) == 'x'.
// Note that lower of non-letters can produce other non-letters.
static byte lower(byte c) {
	return c | ('x' - 'X');
}

// underscoreOK reports whether the underscores in s are allowed.
// Checking them in this one function lets all the parsers skip over them simply.
// Underscore must appear only between digits or between a base prefix and a digit.
static bool underscore_ok(str s) {
	// saw tracks the last character (class) we saw:
	// ^ for beginning of number,
	// 0 for a digit or base prefix,
	// _ for an underscore,
	// ! for none of the above.
	char saw = '^';
	size i = 0;

	// Optional sign.
	if (len(s) >= 1 && (s[0] == '-' || s[0] == '+')) {
		s = s+1;
	}

	// Optional base prefix.
	bool hex = false;
	if (len(s) >= 2 && s[0] == '0' && (lower(s[1]) == 'b' || lower(s[1]) == 'o' || lower(s[1]) == 'x')) {
		i = 2;
		saw = '0'; // base prefix counts as a digit for "underscore as digit separator"
		hex = lower(s[1]) == 'x';
	}

	// Number proper.
	for (; i < len(s); i++) {
		// Digits are always okay.
		if ('0' <= s[i] && s[i] <= '9' || hex && 'a' <= lower(s[i]) && lower(s[i]) <= 'f') {
			saw = '0';
			continue;
		}
		// Underscore must follow digit.
		if (s[i] == '_') {
			if (saw != '0') {
				return false;
			}
			saw = '_';
			continue;
		}
		// Underscore must also be followed by digit.
		if (saw == '_') {
			return false;
		}
		// Saw non-digit, non-underscore.
		saw = '!';
	}
	return saw != '_';
}

int64 strconv::parse_int(str s, int base, int bit_size, error err) {
    const str fn_parse_int = "parse_int";

	if (s == "") {
		syntax_error(err, fn_parse_int, s);
		return 0;
	}

	// Pick off leading sign.
	str s0 = s;
	bool neg = false;
	switch (s[0]) {
	case '+':
		s = s+1;
		break;
	case '-':
		s = s+1;
		neg = true;
		break;
	}

	// Convert unsigned and check range.
	uint64 un = parse_uint(s, base, bit_size, ErrorReporter([&](Error &e) {
		NumError &ne = e.cast<NumError>();
		ne.func = fn_parse_int;
		ne.num = s0;
		err(ne);
	}));
	// if err != nil && err.(*NumError).Err != ErrRange {
	// 	err.(*NumError).Func = fnParseInt
	// 	err.(*NumError).Num = stringslite.Clone(s0)
	// 	return 0, err
	// }

	if (bit_size == 0) {
		bit_size = IntSize;
	}

	uint64 cutoff = uint64(1) << uint(bit_size-1);
	if (!neg && un >= cutoff) {
		range_error(err, fn_parse_int, s0);
		return int64(cutoff - 1);
	}
	if (neg && un > cutoff) {
		range_error(err, fn_parse_int, s0);
		return -int64(cutoff);
	}
	int64 n = int64(un);
	if (neg) {
		n = -n;
	}
	return n;
}

uint64 strconv::parse_uint(str s, int base, int bit_size, error err) {
    const str fn_parse_uint = "parse_uint";

	if (s == "") {
        syntax_error(err, fn_parse_uint, s);
		return 0;
	}

	bool base0 = base == 0;
	str s0 = s;
	
	if (2 <= base && base <= 36) {
		// valid base; nothing to do
    } else if (base == 0) {
		// Look for octal, hex prefix.
		base = 10;
		if (s[0] == '0') {
			if (len(s) >= 3 && lower(s[1]) == 'b') {
				base = 2;
				s = s+2;
            } else if (len(s) >= 3 && lower(s[1]) == 'o') {
				base = 8;
				s = s+2;
			} else if (len(s) >= 3 && lower(s[1]) == 'x') {
				base = 16;
				s = s+2;
            } else {
				base = 8;
				s = s+1;
			}
		}
	} else {
        base_error(err, fn_parse_uint, s0, base);
		return 0;
	}

	if (bit_size == 0) {
		bit_size = IntSize;
	} else if (bit_size < 0 || bit_size > 64) {
		bit_size_error(err, fn_parse_uint, s0, bit_size);
        return 0;
	}

	// Cutoff is the smallest number such that cutoff*base > maxUint64.
	// Use compile-time constants for common cases.
    uint64 cutoff;
	switch (base) {
	case 10:
		cutoff = MaxUint64/10 + 1;
        break;
	case 16:
		cutoff = MaxUint64/16 + 1;
        break;
	default:
		cutoff = MaxUint64/uint64(base) + 1;
        break;
	}

	uint64 max_val = ~uint64(0) >> (64 - bit_size);

	bool underscores = false;
	uint64 n = 0;
	for (byte c : s) {
		byte d;
		
		if (c == '_' && base0) {
			underscores = true;
			continue;
        }

		if ('0' <= c && c <= '9') {
			d = c - '0';
        } else if ('a' <= lower(c) && lower(c) <= 'z') {
			d = lower(c) - 'a' + 10;
		} else {
            syntax_error(err, fn_parse_uint, s0);
			return 0;
		}

		if (d >= byte(base)) {
            syntax_error(err, fn_parse_uint, s0);
			return 0;
		}

		if (n >= cutoff) {
			// n*base overflows
            range_error(err, fn_parse_uint, s0);
			return max_val;
		}
		n *= uint64(base);

		uint64 n1 = n + uint64(d);
		if (n1 < n || n1 > max_val) {
			// n+d overflows
            range_error(err, fn_parse_uint, s0);
			return max_val;
		}
		n = n1;
    }

	if (underscores && !underscore_ok(s0)) {
        syntax_error(err, fn_parse_uint, s0);
		return 0;
	}

	return n;
}

void NumError::fmt(io::Writer &out, error err) const {
	NumError const &e = *this;
    fmt::fprintf(out, err, "strconv::%s: parsing %q: %v", e.func, e.num, e.err);
}

int strconv::atoi(str s, error err) {
	const str fn_atoi = "atoi";

	size slen = len(s);
	if (IntSize == 32 && (0 < slen && slen < 10) ||
		IntSize == 64 && (0 < slen && slen < 19)) {
		// Fast path for small integers that fit int type.
		str s0 = s;
		if (s[0] == '-' || s[0] == '+') {
			s = s+1;
			if (len(s) < 1) {
				syntax_error(err, fn_atoi, s0);
				return 0;
			}
		}

		int n = 0;
		for (byte ch : s) {
			ch -= '0';
			if (ch > 9) {
				syntax_error(err, fn_atoi, s0);
				return 0;
			}
			n = n*10 + int(ch);
		}
		if (s0[0] == '-') {
			n = -n;
		}
		return n;
	}

	// Slow path for invalid, big, or underscored integers.
	int64 i64 = parse_int(s, 10, 0, [&](Error &e){
		e.cast<NumError>().func = fn_atoi;
		err(e);
	});
	return int(i64);
}

view<Error*> NumError::unwrap() const {
	return view<Error*>(&this->err, 1);
}
