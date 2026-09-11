#include "fmt.h"
#include "lib/strings/strings.h"
#include "lib/testing/testing.h"

using namespace lib;

// Report incorrect digits or suffixes through t, including the old buffer boundary.
void test_unicode_large_precision(testing::T &t) {
    // Exercise the old buffer boundary with and without multibyte quoted runes.
    for (int precision : {4, 247, 248, 249, 253, 254, 255, 256, 257, 300, 1024}) {
        String plain = fmt::sprint("U+", strings::repeat("0", precision - 2), "2A");
        String actual = fmt::sprintf("%.*U", precision, 42);
        if (actual != plain) {
            t.errorf("incorrect Unicode digits at precision %d", precision);
        }

        String quoted = fmt::sprint("U+", strings::repeat("0", precision > 5 ? precision - 5 : 0),
                                    "1D6C2 '𝛂'");
        actual = fmt::sprintf("%#.*U", precision, U'𝛂');
        if (actual != quoted) {
            t.errorf("incorrect quoted Unicode digits at precision %d", precision);
        }
    }
}

// Report incorrect width, alignment, or negative-value formatting through t.
void test_unicode_large_precision_width(testing::T &t) {
    // Width counts characters, including one rune for 日, rather than UTF-8 bytes.
    String expected = fmt::sprint("U+", strings::repeat("0", 296), "65E5 '日'");
    String left = fmt::sprint("    ", expected);
    String right = fmt::sprint(expected, "    ");
    for (str format : {str("%#310.300U"), str("%#0310.300U")}) {
        String actual = fmt::sprintf(format, U'日');
        if (actual != left) {
            t.errorf("incorrect left padding for %s", format);
        }
    }
    for (str format : {str("%#-310.300U"), str("%#-0310.300U")}) {
        String actual = fmt::sprintf(format, U'日');
        if (actual != right) {
            t.errorf("incorrect right padding for %s", format);
        }
    }
    String negative = fmt::sprintf("%.300U", -1);
    expected = fmt::sprint("U+", strings::repeat("0", 284), "FFFFFFFFFFFFFFFF");
    if (negative != expected) {
        t.errorf("incorrect negative Unicode value at large precision");
    }
}
