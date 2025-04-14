#include "atoi.h"
#include "lib/error.h"
#include "lib/fmt/fmt.h"
#include "lib/testing/testing.h"

using namespace lib;
using namespace lib::strconv;
using namespace lib::testing;

struct ParseUint64Test {
	str in;
	uint64 out;
	Error *err;
} parse_uint64_tests[]{
	{"", 0, new ErrSyntax()},
	{"0", 0, nil},
	{"1", 1, nil},
	{"12345", 12345, nil},
	{"012345", 12345, nil},
	{"12345x", 0, new ErrSyntax},
	{"98765432100", 98765432100, nil},
	{"18446744073709551615", uint64(-1), nil},
	{"18446744073709551616", uint64(-1), new ErrRange},
	{"18446744073709551620", uint64(-1), new ErrRange},
	{"1_2_3_4_5", 0, new ErrSyntax}, // base=10 so no underscores allowed
	{"_12345", 0, new ErrSyntax},
	{"1__2345", 0, new ErrSyntax},
	{"12345_", 0, new ErrSyntax},
	{"-0", 0, new ErrSyntax},
	{"-1", 0, new ErrSyntax},
	{"+1", 0, new ErrSyntax},
} ;

struct ParseUint64BaseTest {
	str in;
	int base;
	uint64 out;
	Error *err;
} parse_uint64_base_tests[] = {
	{"", 0, 0, new ErrSyntax},
	{"0", 0, 0, nil},
	{"0x", 0, 0, new ErrSyntax},
	{"0X", 0, 0, new ErrSyntax},
	{"1", 0, 1, nil},
	{"12345", 0, 12345, nil},
	{"012345", 0, 012345, nil},
	{"0x12345", 0, 0x12345, nil},
	{"0X12345", 0, 0x12345, nil},
	{"12345x", 0, 0, new ErrSyntax},
	{"0xabcdefg123", 0, 0, new ErrSyntax},
	{"123456789abc", 0, 0, new ErrSyntax},
	{"98765432100", 0, 98765432100, nil},
	{"18446744073709551615", 0, uint64(-1), nil},
	{"18446744073709551616", 0, uint64(-1), new ErrRange},
	{"18446744073709551620", 0, uint64(-1), new ErrRange},
	{"0xFFFFFFFFFFFFFFFF", 0, uint64(-1), nil},
	{"0x10000000000000000", 0, uint64(-1), new ErrRange},
	{"01777777777777777777777", 0, uint64(-1), nil},
	{"01777777777777777777778", 0, 0, new ErrSyntax},
	{"02000000000000000000000", 0, uint64(-1), new ErrRange},
	{"0200000000000000000000", 0, (uint64(1) << 61), nil},
	{"0b", 0, 0, new ErrSyntax},
	{"0B", 0, 0, new ErrSyntax},
	{"0b101", 0, 5, nil},
	{"0B101", 0, 5, nil},
	{"0o", 0, 0, new ErrSyntax},
	{"0O", 0, 0, new ErrSyntax},
	{"0o377", 0, 255, nil},
	{"0O377", 0, 255, nil},

	// underscores allowed with base == 0 only
	{"1_2_3_4_5", 0, 12345, nil}, // base 0 => 10
	{"_12345", 0, 0, new ErrSyntax},
	{"1__2345", 0, 0, new ErrSyntax},
	{"12345_", 0, 0, new ErrSyntax},

	{"1_2_3_4_5", 10, 0, new ErrSyntax}, // base 10
	{"_12345", 10, 0, new ErrSyntax},
	{"1__2345", 10, 0, new ErrSyntax},
	{"12345_", 10, 0, new ErrSyntax},

	{"0x_1_2_3_4_5", 0, 0x12345, nil}, // base 0 => 16
	{"_0x12345", 0, 0, new ErrSyntax},
	{"0x__12345", 0, 0, new ErrSyntax},
	{"0x1__2345", 0, 0, new ErrSyntax},
	{"0x1234__5", 0, 0, new ErrSyntax},
	{"0x12345_", 0, 0, new ErrSyntax},

	{"1_2_3_4_5", 16, 0, new ErrSyntax}, // base 16
	{"_12345", 16, 0, new ErrSyntax},
	{"1__2345", 16, 0, new ErrSyntax},
	{"1234__5", 16, 0, new ErrSyntax},
	{"12345_", 16, 0, new ErrSyntax},

	{"0_1_2_3_4_5", 0, 012345, nil}, // base 0 => 8 (0377)
	{"_012345", 0, 0, new ErrSyntax},
	{"0__12345", 0, 0, new ErrSyntax},
	{"01234__5", 0, 0, new ErrSyntax},
	{"012345_", 0, 0, new ErrSyntax},

	{"0o_1_2_3_4_5", 0, 012345, nil}, // base 0 => 8 (0o377)
	{"_0o12345", 0, 0, new ErrSyntax},
	{"0o__12345", 0, 0, new ErrSyntax},
	{"0o1234__5", 0, 0, new ErrSyntax},
	{"0o12345_", 0, 0, new ErrSyntax},

	{"0_1_2_3_4_5", 8, 0, new ErrSyntax}, // base 8
	{"_012345", 8, 0, new ErrSyntax},
	{"0__12345", 8, 0, new ErrSyntax},
	{"01234__5", 8, 0, new ErrSyntax},
	{"012345_", 8, 0, new ErrSyntax},

	{"0b_1_0_1", 0, 5, nil}, // base 0 => 2 (0b101)
	{"_0b101", 0, 0, new ErrSyntax},
	{"0b__101", 0, 0, new ErrSyntax},
	{"0b1__01", 0, 0, new ErrSyntax},
	{"0b10__1", 0, 0, new ErrSyntax},
	{"0b101_", 0, 0, new ErrSyntax},

	{"1_0_1", 2, 0, new ErrSyntax}, // base 2
	{"_101", 2, 0, new ErrSyntax},
	{"1_01", 2, 0, new ErrSyntax},
	{"10_1", 2, 0, new ErrSyntax},
	{"101_", 2, 0, new ErrSyntax},
};

struct ParseInt64Test {
	str in;
	int64 out;
	Error *err;
} parse_int64_tests[] = {
	{"", 0, new ErrSyntax},
	{"0", 0, nil},
	{"-0", 0, nil},
	{"+0", 0, nil},
	{"1", 1, nil},
	{"-1", -1, nil},
	{"+1", 1, nil},
	{"12345", 12345, nil},
	{"-12345", -12345, nil},
	{"012345", 12345, nil},
	{"-012345", -12345, nil},
	{"98765432100", 98765432100, nil},
	{"-98765432100", -98765432100, nil},
	{"9223372036854775807", (uint64(1)<<63) - 1, nil},
	{"-9223372036854775807", -int64(((uint64(1)<<63) - 1)), nil},
	{"9223372036854775808", (uint64(1)<<63) - 1, new ErrRange},
	{"-9223372036854775808", int64(-1)<<63, nil},
	{"9223372036854775809", (uint64(1)<<63) - 1, new ErrRange},
	{"-9223372036854775809", int64(-1) << 63, new ErrRange},
	{"-1_2_3_4_5", 0, new ErrSyntax}, // base=10 so no underscores allowed
	{"-_12345", 0, new ErrSyntax},
	{"_12345", 0, new ErrSyntax},
	{"1__2345", 0, new ErrSyntax},
	{"12345_", 0, new ErrSyntax},
	{"123%45", 0, new ErrSyntax},
};

struct ParseInt64BaseTest {
	str in;
	int base;
	int64 out;
	Error *err;
} static parse_int64_base_tests[] = {
	{"", 0, 0, new ErrSyntax},
	{"0", 0, 0, nil},
	{"-0", 0, 0, nil},
	{"1", 0, 1, nil},
	{"-1", 0, -1, nil},
	{"12345", 0, 12345, nil},
	{"-12345", 0, -12345, nil},
	{"012345", 0, 012345, nil},
	{"-012345", 0, -012345, nil},
	{"0x12345", 0, 0x12345, nil},
	{"-0X12345", 0, -0x12345, nil},
	{"12345x", 0, 0, new ErrSyntax},
	{"-12345x", 0, 0, new ErrSyntax},
	{"98765432100", 0, 98765432100, nil},
	{"-98765432100", 0, -98765432100, nil},
	{"9223372036854775807", 0, (uint64(1)<<63) - 1, nil},
	{"-9223372036854775807", 0, -int64((uint64(1)<<63) - 1), nil},
	{"9223372036854775808", 0, (uint64(1)<<63) - 1, new ErrRange},
	{"-9223372036854775808", 0, int64(-1)<<63, nil},
	{"9223372036854775809", 0, (uint64(1)<<63) - 1, new ErrRange},
	{"-9223372036854775809", 0, int64(-1)<<63, new ErrRange},

	// other bases
	{"g", 17, 16, nil},
	{"10", 25, 25, nil},
	{"holycow", 35, (((((17ul*35+24)*35+21)*35+34)*35+12)*35+24)*35 + 32, nil},
	{"holycow", 36, (((((17ul*36+24)*36+21)*36+34)*36+12)*36+24)*36 + 32, nil},

	// base 2
	{"0", 2, 0, nil},
	{"-1", 2, -1, nil},
	{"1010", 2, 10, nil},
	{"1000000000000000", 2, 1 << 15, nil},
	{"111111111111111111111111111111111111111111111111111111111111111", 2, (uint64(1)<<63) - 1, nil},
	{"1000000000000000000000000000000000000000000000000000000000000000", 2, (uint64(1)<<63) - 1, new ErrRange},
	{"-1000000000000000000000000000000000000000000000000000000000000000", 2, int64(-1)<<63, nil},
	{"-1000000000000000000000000000000000000000000000000000000000000001", 2, int64(-1)<<63, new ErrRange},

	// base 8
	{"-10", 8, -8, nil},
	{"57635436545", 8, 057635436545, nil},
	{"100000000", 8, 1 << 24, nil},

	// base 16
	{"10", 16, 16, nil},
	{"-123456789abcdef", 16, -0x123456789abcdef, nil},
	{"7fffffffffffffff", 16, (uint64(1)<<63) - 1, nil},

	// underscores
	{"-0x_1_2_3_4_5", 0, -0x12345, nil},
	{"0x_1_2_3_4_5", 0, 0x12345, nil},
	{"-_0x12345", 0, 0, new ErrSyntax},
	{"_-0x12345", 0, 0, new ErrSyntax},
	{"_0x12345", 0, 0, new ErrSyntax},
	{"0x__12345", 0, 0, new ErrSyntax},
	{"0x1__2345", 0, 0, new ErrSyntax},
	{"0x1234__5", 0, 0, new ErrSyntax},
	{"0x12345_", 0, 0, new ErrSyntax},

	{"-0_1_2_3_4_5", 0, -012345, nil}, // octal
	{"0_1_2_3_4_5", 0, 012345, nil},   // octal
	{"-_012345", 0, 0, new ErrSyntax},
	{"_-012345", 0, 0, new ErrSyntax},
	{"_012345", 0, 0, new ErrSyntax},
	{"0__12345", 0, 0, new ErrSyntax},
	{"01234__5", 0, 0, new ErrSyntax},
	{"012345_", 0, 0, new ErrSyntax},

	{"+0xf", 0, 0xf, nil},
	{"-0xf", 0, -0xf, nil},
	{"0x+f", 0, 0, new ErrSyntax},
	{"0x-f", 0, 0, new ErrSyntax},
} ;

struct ParseUint32Test {
	str in;
	uint32 out;
	Error *err;
} parse_uint32_tests[] = {
	{"", 0, new ErrSyntax},
	{"0", 0, nil},
	{"1", 1, nil},
	{"12345", 12345, nil},
	{"012345", 12345, nil},
	{"12345x", 0, new ErrSyntax},
	{"987654321", 987654321, nil},
	{"4294967295", (uint64(1)<<32) - 1, nil},
	{"4294967296", (uint64(1)<<32) - 1, new ErrRange},
	{"1_2_3_4_5", 0, new ErrSyntax}, // base=10 so no underscores allowed
	{"_12345", 0, new ErrSyntax},
	{"_12345", 0, new ErrSyntax},
	{"1__2345", 0, new ErrSyntax},
	{"12345_", 0, new ErrSyntax},
} ;

struct ParseInt32Test {
	str in;
	int32 out;
	Error *err;
} parse_int32_tests[] = {
	{"", 0, new ErrSyntax},
	{"0", 0, nil},
	{"-0", 0, nil},
	{"1", 1, nil},
	{"-1", -1, nil},
	{"12345", 12345, nil},
	{"-12345", -12345, nil},
	{"012345", 12345, nil},
	{"-012345", -12345, nil},
	{"12345x", 0, new ErrSyntax},
	{"-12345x", 0, new ErrSyntax},
	{"987654321", 987654321, nil},
	{"-987654321", -987654321, nil},
	{"2147483647", (1u<<31) - 1, nil},
	{"-2147483647", -int((1u<<31) - 1), nil},
	{"2147483648", (1u<<31) - 1, new ErrRange},
	{"-2147483648", -1 << 31, nil},
	{"2147483649", (1u<<31) - 1, new ErrRange},
	{"-2147483649", -1 << 31, new ErrRange},
	{"-1_2_3_4_5", 0, new ErrSyntax}, // base=10 so no underscores allowed
	{"-_12345", 0, new ErrSyntax},
	{"_12345", 0, new ErrSyntax},
	{"1__2345", 0, new ErrSyntax},
	{"12345_", 0, new ErrSyntax},
	{"123%45", 0, new ErrSyntax},
};

struct NumErrorTest {
	str num, want;
} num_error_tests[] = {
	{"0", R"(strconv::parse_float: parsing "0": failed)"},
	{"`", "strconv::parse_float: parsing \"`\": failed"},
	{"1\x00.2", R"(strconv::parse_float: parsing "1\x00.2": failed)"},
};

void test_parse_uint32(T &t) {
	for (auto const &test : parse_uint32_tests) {
		ErrorRecorder err;
		ErrorRecorder expected_err;

		uint64 out = parse_uint(test.in, 10, 32, err);
		if (test.err != nil) {
			expected_err.report(NumError("parse_uint", test.in, *test.err));
		}

		if (uint64(test.out) != out || err.has_error != expected_err.has_error || err.msg != expected_err.msg) {
			t.errorf("parse_uint(%q, 10, 32) = %v, <%v> want %v, <%v>",
				test.in, out, err, test.out, expected_err);
		}
	}
}

void test_parse_uint64(T &t) {
	for (auto const &test : parse_uint64_tests) {
        ErrorRecorder err;
		ErrorRecorder expected_err;

		uint64 out = parse_uint(test.in, 10, 64, err);
		if (test.err != nil) {
			expected_err.report(NumError("parse_uint", test.in, *test.err));
		}
		
		if (test.out != out || err.has_error != expected_err.has_error || err.msg != expected_err.msg) {
			t.errorf("parse_uint(%q, 10, 64) = %v, <%v> want %v, <%v>",
				test.in, out, err, test.out, expected_err);
		}
	}
}

void test_parse_uint64_base(T &t) {
	for (auto const &test : parse_uint64_base_tests) {
        ErrorRecorder err;
		ErrorRecorder expected_err;

		uint64 out = parse_uint(test.in, test.base, 64, err);
		if (test.err != nil) {
			expected_err.report(NumError("parse_uint", test.in, *test.err));
		}
		
		if (test.out != out || err.has_error != expected_err.has_error || err.msg != expected_err.msg) {
			t.errorf("parse_uint(%q, %v, 64) = %v, <%v> want %v, <%v>",
				test.in, test.base, out, err, test.out, expected_err);
		}
	}
}


void test_parse_int32(T &t) {
	for (auto const &test : parse_int32_tests) {
        ErrorRecorder err;
		ErrorRecorder expected_err;

		int64 out = parse_int(test.in, 10, 32, err);
		if (test.err != nil) {
			expected_err.report(NumError("parse_int", test.in, *test.err));
		}
		
		if (test.out != out || err.has_error != expected_err.has_error || err.msg != expected_err.msg) {
			t.errorf("parse_int(%q, %v, 32) = %v, <%v> want %v, <%v>",
				test.in, 10, out, err, test.out, expected_err);
		}
	}
}

void test_parse_int64(T &t) {
	for (auto const &test : parse_int64_tests) {
        ErrorRecorder err;
		ErrorRecorder expected_err;

		int64 out = parse_int(test.in, 10, 64, err);
		if (test.err != nil) {
			expected_err.report(NumError("parse_int", test.in, *test.err));
		}
		
		if (test.out != out || err.has_error != expected_err.has_error || err.msg != expected_err.msg) {
			t.errorf("parse_int(%q, %v, 64) = %v, <%v> want %v, <%v>",
				test.in, 10, out, err, test.out, expected_err);
		}
	}
}

void test_parse_int64_base(T &t) {
	for (auto const &test : parse_int64_base_tests) {
        ErrorRecorder err;
		ErrorRecorder expected_err;

		int64 out = parse_int(test.in, test.base, 64, err);
		if (test.err != nil) {
			expected_err.report(NumError("parse_int", test.in, *test.err));
		}
		
		if (test.out != out || err.has_error != expected_err.has_error || err.msg != expected_err.msg) {
			t.errorf("parse_int(%q, %v, %v) = %v, <%v> want %v, <%v>",
				test.in, test.base, out, err, test.out, expected_err);
		}
	}
}

void test_parse_uint(T &t) {
	switch (IntSize) {
	case 32:
		for (auto &&test : parse_uint32_tests) {
			ErrorRecorder err;
			ErrorRecorder expected_err;
			uint64 out = parse_uint(test.in, 10, 0, err);
			if (test.err != nil) {
				expected_err.report(NumError("parse_uint", test.in, *test.err));
			}
			if (uint64(test.out) != out || err.has_error != expected_err.has_error || err.msg != expected_err.msg) {
				t.errorf("parse_uint(%q, 10, 0) = %v, <%v> want %v, <%v>",
					test.in, out, err, test.out, expected_err);
			}
		}
		break;
	case 64:
		for (auto &&test : parse_uint64_tests) {
			ErrorRecorder err;
			ErrorRecorder expected_err;
			uint64 out = parse_uint(test.in, 10, 0, err);
			if (test.err != nil) {
				expected_err.report(NumError("parse_uint", test.in, *test.err));
			}
			if (test.out != out || err.has_error != expected_err.has_error || err.msg != expected_err.msg) {
				t.errorf("parse_uint(%q, 10, 0) = %v, <%v> want %v, <%v>",
					test.in, out, err, test.out, expected_err);
			}
		}
		break;
	}
}


void test_parse_int(T &t) {
	switch (IntSize) {
	case 32:
		for (auto &&test : parse_int32_tests) {
			ErrorRecorder err;
			ErrorRecorder expected_err;
			uint64 out = parse_int(test.in, 10, 0, err);
			if (test.err != nil) {
				expected_err.report(NumError("parse_int", test.in, *test.err));
			}
			if (uint64(test.out) != out || err.has_error != expected_err.has_error || err.msg != expected_err.msg) {
				t.errorf("parse_int(%q, 10, 0) = %v, <%v> want %v, <%v>",
					test.in, out, err, test.out, expected_err);
			}
		}
		break;
	case 64:
		for (auto &&test : parse_int64_tests) {
			ErrorRecorder err;
			ErrorRecorder expected_err;
			uint64 out = parse_int(test.in, 10, 0, err);
			if (test.err != nil) {
				expected_err.report(NumError("parse_int", test.in, *test.err));
			}
			if (test.out != out || err.has_error != expected_err.has_error || err.msg != expected_err.msg) {
				t.errorf("parse_int(%q, 10, 0) = %v, <%v> want %v, <%v>",
					test.in, out, err, test.out, expected_err);
			}
		}
		break;
	}
}

void test_atoi(T &t) {
	switch (IntSize) {
	case 32:
		for (auto &&test : parse_int32_tests) {
			ErrorRecorder err;
			ErrorRecorder expected_err;
			uint64 out = atoi(test.in, err);
			if (test.err != nil) {
				expected_err.report(NumError("atoi", test.in, *test.err));
			}
			if (uint64(test.out) != out || err.has_error != expected_err.has_error || err.msg != expected_err.msg) {
				t.errorf("atoi(%q) = %v, <%v> want %v, <%v>", test.in, out, err, test.out, expected_err);
			}
		}
		break;
	case 64:
		for (auto &&test : parse_int64_tests) {
			ErrorRecorder err;
			ErrorRecorder expected_err;
			uint64 out = atoi(test.in, err);
			if (test.err != nil) {
				expected_err.report(NumError("atoi", test.in, *test.err));
			}
			if (test.out != out || err.has_error != expected_err.has_error || err.msg != expected_err.msg) {
				t.errorf("atoi(%q) = %v, <%v> want %v, <%v>", test.in, out, err, test.out, expected_err);
			}
		}
		break;
	}
}

void bit_size_err_stub(error err, str name, int bit_size) {
	err(NumError(name, "0", fmt::errorf("invalid bit size %v", bit_size)));
}

void base_err_stub(error err, str name, int base) {
	err(NumError(name, "0", fmt::errorf("invalid base %v", base)));
}

void no_err_stub(error, str, int) {
	// do nothing
}

struct ParseErrorTest {
	int arg;
	std::function<void(error err, str name, int arg)> err_stub;
} ;

ParseErrorTest parse_bit_size_tests[] = {
	{-1, bit_size_err_stub},
	{0, no_err_stub},
	{64, no_err_stub},
	{65, bit_size_err_stub},
};

ParseErrorTest parse_base_tests[] = {
	{-1, base_err_stub},
	{0, no_err_stub},
	{1, base_err_stub},
	{2, no_err_stub},
	{36, no_err_stub},
	{37, base_err_stub},
};

// func equalError(a, b error) bool {
// 	if a == nil {
// 		return b == nil
// 	}
// 	if b == nil {
// 		return a == nil
// 	}
// 	return a.Error() == b.Error()
// }

void test_parse_int_bit_size(T &t) {
	for (auto &&test : parse_bit_size_tests) {
		ErrorRecorder want_err, got_err;
		test.err_stub(want_err, "parse_int", test.arg);
		parse_int("0", 0, test.arg, got_err);

		if (want_err.has_error != got_err.has_error || want_err.msg != got_err.msg) {
			t.errorf("parse_int(\"0\", 0, %v) = 0, %v want 0, %v", test.arg, got_err, want_err);
		}
	}
}

void test_parse_uint_bit_size(T &t) {
	for (auto &&test : parse_bit_size_tests) {
		ErrorRecorder want_err, got_err;
		test.err_stub(want_err, "parse_uint", test.arg);
		parse_uint("0", 0, test.arg, got_err);

		if (want_err.has_error != got_err.has_error || want_err.msg != got_err.msg) {
			t.errorf("parse_uint(\"0\", 0, %v) = 0, %v want 0, %v", test.arg, got_err, want_err);
		}
	}
}

void test_parse_int_base(T &t) {
	for (auto &&test : parse_base_tests) {
		ErrorRecorder want_err, got_err;
		test.err_stub(want_err, "parse_int", test.arg);
		parse_int("0", test.arg, 0, got_err);

		if (want_err.has_error != got_err.has_error || want_err.msg != got_err.msg) {
			t.errorf("parse_int(\"0\", %v, 0) = 0, %v want 0, %v", test.arg, got_err, want_err);
		}
	}
}

void test_parse_uint_base(T &t) {
	for (auto &&test : parse_base_tests) {
		ErrorRecorder want_err, got_err;
		test.err_stub(want_err, "parse_uint", test.arg);
		parse_uint("0", test.arg, 0, got_err);

		if (want_err.has_error != got_err.has_error || want_err.msg != got_err.msg) {
			t.errorf("parse_uint(\"0\", %v, 0) = 0, %v want 0, %v", test.arg, got_err, want_err);
		}
	}
}

void test_num_error(T &t) {
	for (auto &&test : num_error_tests) {
		BasicError failed("failed");
		NumError err(
			"parse_float",
			test.num,
			failed);
		if (String got = fmt::sprint(err); got != test.want) {
			t.errorf(R"(String(NumError("parse_float", %q, "failed")) = <%v>, want <%v>)", test.num, got, test.want);
		}
	}
}

void test_num_error_unwrap(T &t) {
	ErrSyntax err_syntax;
	NumError err("", "", err_syntax);
	err.init();
	if (!err.is<ErrSyntax>()) {
		t.error("NumError::is<ErrSyntax> failed, wanted success");
	}
}

// func BenchmarkParseInt(b *testing.B) {
// 	b.Run("Pos", func(b *testing.B) {
// 		benchmarkParseInt(b, 1)
// 	})
// 	b.Run("Neg", func(b *testing.B) {
// 		benchmarkParseInt(b, -1)
// 	})
// }

// type benchCase struct {
// 	name string
// 	num  int64
// }

// func benchmarkParseInt(b *testing.B, neg int) {
// 	cases := []benchCase{
// 		{"7bit", 1<<7 - 1},
// 		{"26bit", 1<<26 - 1},
// 		{"31bit", 1<<31 - 1},
// 		{"56bit", 1<<56 - 1},
// 		{"63bit", 1<<63 - 1},
// 	}
// 	for _, cs := range cases {
// 		b.Run(cs.name, func(b *testing.B) {
// 			s := fmt.Sprintf("%d", cs.num*int64(neg))
// 			for i := 0; i < b.N; i++ {
// 				out, _ := ParseInt(s, 10, 64)
// 				BenchSink += int(out)
// 			}
// 		})
// 	}
// }

// func BenchmarkAtoi(b *testing.B) {
// 	b.Run("Pos", func(b *testing.B) {
// 		benchmarkAtoi(b, 1)
// 	})
// 	b.Run("Neg", func(b *testing.B) {
// 		benchmarkAtoi(b, -1)
// 	})
// }

// func benchmarkAtoi(b *testing.B, neg int) {
// 	cases := []benchCase{
// 		{"7bit", 1<<7 - 1},
// 		{"26bit", 1<<26 - 1},
// 		{"31bit", 1<<31 - 1},
// 	}
// 	if IntSize == 64 {
// 		cases = append(cases, []benchCase{
// 			{"56bit", 1<<56 - 1},
// 			{"63bit", 1<<63 - 1},
// 		}...)
// 	}
// 	for _, cs := range cases {
// 		b.Run(cs.name, func(b *testing.B) {
// 			s := fmt.Sprintf("%d", cs.num*int64(neg))
// 			for i := 0; i < b.N; i++ {
// 				out, _ := Atoi(s)
// 				BenchSink += out
// 			}
// 		})
// 	}
// }