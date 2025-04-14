#include "itoa.h"

#include "deps/fmt/include/fmt/base.h"
#include "deps/fmt/include/fmt/format.h"
#include "lib/exceptions.h"
#include "lib/io/io_stream.h"
#include "lib/strconv/quote.h"
#include "lib/fmt/fmt.h"
#include "lib/testing/testing.h"

using namespace lib;
using namespace lib::strconv;
using namespace lib::strconv::detail;


struct Itob64Test {
	int64 in;  
	int base;
	String out;
} ;

const Itob64Test itob64tests[] = {
	{0, 10, "0"},
	{1, 10, "1"},
	{-1, 10, "-1"},
	{12345678, 10, "12345678"},
	{-987654321, 10, "-987654321"},
	{(1l<<31) - 1, 10, "2147483647"},
	{(-1l<<31) + 1, 10, "-2147483647"},
	{1l << 31, 10, "2147483648"},
	{-1 << 31, 10, "-2147483648"},
	{(1l<<31) + 1, 10, "2147483649"},
	{(-1l<<31) - 1, 10, "-2147483649"},
	{(1l<<32) - 1, 10, "4294967295"},
	{(-1l<<32) + 1, 10, "-4294967295"},
	{1l << 32, 10, "4294967296"},
	{-1l << 32, 10, "-4294967296"},
	{(1l<<32) + 1, 10, "4294967297"},
	{(-1l<<32) - 1, 10, "-4294967297"},
	{1l << 50, 10, "1125899906842624"},
	{(1ul<<63) - 1, 10, "9223372036854775807"},
	{(-1l<<63) + 1, 10, "-9223372036854775807"},
	{(-1l << 63), 10, "-9223372036854775808"},

	{0, 2, "0"},
	{10, 2, "1010"},
	{-1, 2, "-1"},
	{1 << 15, 2, "1000000000000000"},

	{-8, 8, "-10"},
	{057635436545, 8, "57635436545"},
	{1 << 24, 8, "100000000"},

	{16, 16, "10"},
	{-0x123456789abcdef, 16, "-123456789abcdef"},
	{(1ul<<63) - 1, 16, "7fffffffffffffff"},
	{(1ul<<63) - 1, 2, "111111111111111111111111111111111111111111111111111111111111111"},
	{-1l << 63, 2, "-1000000000000000000000000000000000000000000000000000000000000000"},

	{16, 17, "g"},
	{25, 25, "10"},
	{(((((17l*35+24)*35+21)*35+34)*35+12)*35+24)*35 + 32, 35, "holycow"},
	{(((((17l*36+24)*36+21)*36+34)*36+12)*36+24)*36 + 32, 36, "holycow"},
};

void test_itoa(testing::T &t) {
    int i = 0;
	for (Itob64Test const &test : itob64tests) {
		String s = format_int(test.in, test.base);
		if (s != test.out) {
			t.errorf("case %d: format_int(%v, %v) = %v want %v",
				i, test.in, test.base, s, test.out);
		}
		
		if (test.in >= 0) {
			String s = format_uint(uint64(test.in), test.base);
			if (s != test.out) {
				t.errorf("case %d: format_uint(%v, %v) = %v want %v",
					i, test.in, test.base, s, test.out);
			}
		}

		if (test.base == 10 && int64(int(test.in)) == test.in) {
			String s = itoa(int(test.in));
			if (s != test.out) {
				t.errorf("case %d: itoa(%v) = %v want %v",
					i, test.in, s, test.out);
			}
		}
        i++;
	}

	// Override when base is illegal
    try {
        format_uint(12345678, 1);
        t.fatalf("expected panic due to illegal base");
    } catch (lib::exceptions::Panic const&) {
        // do nothing
    }	
}

struct Uitob64Test {
	uint64 in;
	int base;
	String out;
} ;

Uitob64Test uitob64tests[] = {
	{(1ul<<63) - 1, 10, "9223372036854775807"},
	{1ul << 63, 10, "9223372036854775808"},
	{(1ul<<63) + 1, 10, "9223372036854775809"},
	{~0ul - 1, 10, "18446744073709551614"},
	{~0ul, 10, "18446744073709551615"},
	{~0ul, 2, "1111111111111111111111111111111111111111111111111111111111111111"},
};

void test_uitoa(testing::T &t) {
    int i = 0;
	for (Uitob64Test const &test : uitob64tests) {
		String s = format_uint(test.in, test.base);
		if (s != test.out) {
			t.errorf("case %d: format_uint(%v, %v) = %v want %v",
				i, test.in, test.base, s, test.out);
		}
        i++;
	}
}

const struct {
	uint64 in;
	String out;
} varlen_uints[] = {
	{1, "1"},
	{12, "12"},
	{123, "123"},
	{1234, "1234"},
	{12345, "12345"},
	{123456, "123456"},
	{1234567, "1234567"},
	{12345678, "12345678"},
	{123456789, "123456789"},
	{1234567890, "1234567890"},
	{12345678901, "12345678901"},
	{123456789012, "123456789012"},
	{1234567890123, "1234567890123"},
	{12345678901234, "12345678901234"},
	{123456789012345, "123456789012345"},
	{1234567890123456, "1234567890123456"},
	{12345678901234567, "12345678901234567"},
	{123456789012345678, "123456789012345678"},
	{1234567890123456789, "1234567890123456789"},
	{12345678901234567890ul, "12345678901234567890"},
};

void test_format_uint_var_len(testing::T &t) {
    int i = 0;
	for (auto const& test : varlen_uints) {
		String s = format_uint(test.in, 10);
		if (s != test.out) {
			t.errorf("case %d: format_uint(%v, 10) = %v want %v", i, test.in, s, test.out);
		}
        i++;
	}
}