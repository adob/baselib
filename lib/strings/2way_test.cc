#include "2way.h"
#include "lib/testing/testing.h"

#include "lib/print.h"
#include <random>

using namespace lib;
using namespace lib::strings;
using namespace lib::strings::detail;

static String reverse(str s) {
    String out;

    for (size i = 0, j = len(s)-1; i < len(s); i++, j--) {
        out += s[j];
    }
    
    return out;
}

static size simple_index(str s, str sep) {
	size n = len(sep);
	for (size i = n; i <= len(s); i++) {
		if (s[i-n,i] == sep) {
			return i - n;
		}
	}
	return -1;
}

struct {
	str s;
	size index;
	size period;
} critical_factorization_tests[] = {
	{"GCAGAGAG", 2, 2},
	{"aab", 2, 1},
	{"aabaa", 2, 3},
	{"abaaaba", 2, 4},
	{"banana", 2, 2},
	{"zanana", 1, 2},
	{"caaaaaaaaacc", 10, 1},
	{"aabaab", 2, 3},
	{"baabaa", 1, 3},
	{"babbab", 2, 3},
	{"abca", 2, 2,},
	{"acba", 1, 3},
    {"ofofo", 1, 2},
    {"barfoofoo", 2, 7},
	// {"aababb", 4, 3}
};

void test_critical_factorization_fwd(testing::T &t) {
	int i = 0;
	for (auto const &tt : critical_factorization_tests) {
		auto [index, period] = critical_factorization_fwd(tt.s);
		if (index != tt.index || period != tt.period) {
			t.errorf("case %v: critical_factorization_fwd(%q) got index=%v, period=%v; want index=%v, period=%v", i, tt.s, index, period, tt.index, tt.period);
		}
		i++;
	}
}

void test_critical_factorization_rev(testing::T &t) {
    int i = 0;
    for (auto const &tt : critical_factorization_tests) {
        String s = reverse(tt.s);
        size want_index = len(s) - tt.index;
        size want_period = tt.period;

        auto [index, period] = critical_factorization_rev(s);

        if (index != want_index || period != want_period) {
			t.errorf("case %v: critical_factorization_rev(%q) got index=%v, period=%v; want index=%v, period=%v", i, s, index, period, want_index, want_period);
		}
		i++;
    }
}

struct LastIndexTest {
    str haystack;
    str needle;
    size index;
} last_index_tests[] = {
    {"xofofo", "ofofo", 1},
    {"xoofoofoofox", "oofoofoo", 1},
    {"xoofoofooxofoofoofox", "oofoofoo", 1},
    {"xoofoofoooofoofox", "oofoofoo", 1},
    {"xoofoofooofoo", "oofoofoo", 1},
    
    // {"xbarfoofoo", "barfoofoo", 1},
    {"xoofoofrab", "oofoofrab", 1},
    {"xoofoofrabfrab", "oofoofrab", 1},
    {"xoofoofrafraf", "oofoofraf", 1}
};

void test_two_way_search(testing::T &t) {
    int i = 0;
    for (auto const &tt : last_index_tests) {
        if (size index = two_way_search_rev(tt.haystack, tt.needle); index != tt.index) {
            t.errorf("case %v: two_way_search_rev(%q, %q) got %v; want %v", i, tt.haystack, tt.needle, index, tt.index);
        }

        String haystack_rev = reverse(tt.haystack);
        String needle_rev = reverse(tt.needle);
        size want_index = len(tt.haystack) - tt.index - len(tt.needle);
        if (size index = two_way_search_fwd(haystack_rev, needle_rev); index != want_index) {
            t.errorf("case %v: two_way_search_fwd(%q, %q) got %v; want %v", i, tt.haystack, tt.needle, index, tt.index);
        }

        i++;
    }
}

void test_index_random(testing::T &t) {
	const str chars = "abcdefghijklmnopqrstuvwxyz0123456789";

	std::random_device rd;  // Non-deterministic seed source
	std::mt19937 gen(rd()); // Mersenne Twister RNG

	auto rand_intn = [&](int n) {
		std::uniform_int_distribution<> distrib(0, n - 1); // Inclusive range [0, N-1]
		return distrib(gen);
	};

	for (int times = 0; times < 10; times++) {
		for (int str_len = 5 + rand_intn(5); str_len < 140; str_len += 10) { // Arbitrary
			String s;
			for (int i = 0; i < str_len; i++) {
				s.append(chars[rand_intn(len(chars))]);
			}
			for (int i = 0; i < 50; i++) {
				int begin = rand_intn(int(len(s) + 1));
				int end = begin + rand_intn(int(len(s)+1-begin));
				String sep = s[begin,end];
				if (i%4 == 0) {
					int pos = rand_intn(int(len(sep) + 1));
					sep = sep[0,pos] + "A" + (sep+pos);
				}

                if (len(s) < 3 || len(sep) < 2) {
                    continue;
                }

				size want = simple_index(s, sep);
				size res = two_way_search_fwd(s, sep);
				if (res != want) {
					t.errorf("index(%q,%q) = %d; want %d", s, sep, res, want);
				}

				size want_rev = -1;
				if (want != -1) {
					want_rev = len(s) - want - len(sep);
				}
				String s_rev = reverse(s);
				String sep_rev = reverse(sep);
				size res_rev = two_way_search_rev(s_rev, sep_rev);
				if (res_rev != want_rev) {
					t.errorf("last_index(%q,%q) = %d; want %d", s_rev, sep_rev, res_rev, want_rev);
				}
			}
		}
	}
}