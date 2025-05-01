#include "itoa.h"
#include <bit>
#include <type_traits>

using namespace lib;
using namespace lib::strconv;
using namespace lib::strconv::detail;

constexpr bool fast_smalls = true;
constexpr int n_smalls = 100;
const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
constexpr bool const host_32bit = sizeof(uintptr) == 4;

const char smalls_string[] = "00010203040506070809"
        "10111213141516171819"
        "20212223242526272829"
        "30313233343536373839"
        "40414243444546474849"
        "50515253545556575859"
        "60616263646566676869"
        "70717273747576777879"
        "80818283848586878889"
        "90919293949596979899";

static bool is_power_of_two(int x) {
	return (x&(x-1)) == 0;
}

template <typename T>
static void format_bits_impl(io::Writer &out, T u, int base, bool neg, error err) {
    byte a[64+1]; // +1 for sign of 64bit value in base 2
	size i = len(a);

	if (neg) {
		u = -u;
	}

	// convert bits
	// We use uint values where we can because those will
	// fit into a single register even on a 32bit machine.
	if (base == 10) {
		// common case: use constants for / because
		// the compiler can optimize it into a multiply+shift

		if constexpr (host_32bit) {
			// convert the lower digits using 32bit operations
			while (u >= 1'000'000'000) {
				// Avoid using r = a%b in addition to q = a/b
				// since 64bit division and modulo operations
				// are calculated by runtime functions on 32bit machines.
				T q = u / 1'000'000'000;
				uint us = uint(u - q*1'000'000'000); // u % 1e9 fits into a uint
				for (int j = 4; j > 0; j--) {
					uint is = us % 100 * 2;
					us /= 100;
					i -= 2;
					a[i+1] = smalls_string[is+1];
					a[i+0] = smalls_string[is+0];
				}

				// us < 10, since it contains the last digit
				// from the initial 9-digit us.
				i--;
				a[i] = smalls_string[us*2+1];

				u = q;
			}
			// u < 1e9
		}

		// u guaranteed to fit into a uint
		uintmax us = u;
		while (us >= 100) {
			uintmax is = us % 100 * 2;
			us /= 100;
			i -= 2;
			a[i+1] = smalls_string[is+1];
			a[i+0] = smalls_string[is+0];
		}

		// us < 100
		uintmax is = us * 2;
		i--;
		a[i] = smalls_string[is+1];
		if (us >= 10) {
			i--;
			a[i] = smalls_string[is];
		}

	} else if (is_power_of_two(base)) {
		// Use shifts and masks instead of / and %.
		//uint shift := uint(bits.TrailingZeros(uint(base)))
        uint shift = std::countr_zero(uint(base));
		T b = T(base);
		uint m = uint(base) - 1; // == 1<<shift - 1
		while (u >= b) {
			i--;
			a[i] = digits[uint(u)&m];
			u >>= shift;
		}
		// u < base
		i--;
		a[i] = digits[uint(u)];
	} else {
		// general case
		T b = uint64(base);
		while (u >= b) {
			i--;
			// Avoid using r = a%b in addition to q = a/b
			// since 64bit division and modulo operations
			// are calculated by runtime functions on 32bit machines.
			T q = u / b;
			a[i] = digits[uint(u-q*b)];
			u = q;
		}
		// u < base
		i--;
		a[i] = digits[uint(u)];
	}

	// add sign, if any
	if (neg) {
		i--;
		a[i] = '-';
	}

    out.write(str(a+i, len(a)-i), err);

	// if append_ {
	// 	d = append(dst, a[i:]...)
	// 	return
	// }
	// s = string(a[i:])
	// return
}

 // small returns the string for an i with 0 <= i < nSmalls.
static void small(io::Writer &out, int i, error err) {
	if (i < 10) {
		out.write(digits[i], err);
        return;
	}
	
    out.write(str(smalls_string+(i*2), 2), err);
}

void strconv::detail::format_bits(io::Writer &out, uintmax u, int base, bool neg, error err) {
    if constexpr (sizeof(uintptr) == sizeof(uint64)) {
        return format_bits_impl(out, uint64(u), base, neg, err);
    } else {
        return format_bits_impl(out, u, base, neg, err);
    }
    
}

template <typename T>
Formatter<T>::Formatter(T t, int base) : val(t), base(base) {}

template <typename T>
void Formatter<T>::write_to(io::Writer &out, error err) const {
    if (fast_smalls && 0 <= val && val < n_smalls && base == 10) {
		small(out, int(val), err);
        return;
	}

	format_bits(out, static_cast<std::make_unsigned_t<T>>(val), base, val < 0, err);
}

Formatter<intmax> strconv::format_int(int64 i, int base) {
    if (base < 2 || base > len(digits)) {
		panic("strconv: illegal format_int base");
	}
    return Formatter<intmax>(i, base);
}

Formatter<uintmax> strconv::format_uint(uint64 i, int base) {
    if (base < 2 || base > len(digits)) {
		panic("strconv: illegal format_int base");
	}
    return Formatter<uintmax>(i, base);
}

Formatter<intmax> strconv::itoa(intmax i) {
    return format_int(i, 10);
}



