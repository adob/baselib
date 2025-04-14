#include "matcher.h"
#include "lib/error.h"
#include "lib/print.h"
#include "lib/strconv/atoi.h"
#include "lib/strconv/quote.h"
#include "lib/strings/strings.h"
#include "lib/utf8/decode.h"
#include "lib/strconv/isprint.h"
#include "lib/utf8/encode.h"
#include "lib/utf8/utf8.h"
#include <memory>

using namespace lib;
using namespace lib::testing;
using namespace lib::testing::detail;

// TODO: fix test_main to avoid race and improve caching, also allowing to
// eliminate this Mutex.
static sync::Mutex match_mutex;

static bool is_space(rune r) {
	if (r < 0x2000) {
		switch (r) {
		// Note: not the same as Unicode Z class.
		case '\t':
		case '\n':
		case '\v':
		case '\f':
		case '\r':
		case ' ':
		case 0x85:
		case 0xA0:
		case 0x1680:
			return true;
		}
	} else {
		if (r <= 0x200a) {
			return true;
		}
		switch (r) {
		case 0x2028:
		case 0x2029:
		case 0x202f:
		case 0x205f:
		case 0x3000:
			return true;
		}
	}
	return false;
}

// rewrite rewrites a subname to having only printable characters and no white
// space.
static String rewrite(str s) {
	String out;
	// b := []byte{}
	for (auto [i, r] : utf8::runes(s)) {
		if (is_space(r)) {
			out += '_';
		} else if (!strconv::is_print(r)) {
			String q = strconv::quote_rune(r);
			out += q[1, len(q)-1];
		} else {
			Array<byte, utf8::MaxRune> buf;
			out += utf8::encode(buf, r);;
		}
	}
	return out;
}

std::tuple<String, bool /*ok*/, bool /*partial*/> Matcher::full_name(Common const *c, str subname) {
    Matcher &m = *this;
    String name = subname;

    sync::Lock lock(m.mu);

	if (c != nil && c->level > 0) {
		name = m.unique(c->name, rewrite(subname));
	}

    sync::Lock match_lock(match_mutex);

	// We check the full array of paths each time to allow for the case that a pattern contains a '/'.
	std::vector<str> elem = strings::split(name, "/");

	// filter must match.
	// accept partial match that may produce full match later.
	auto [ok, partial] = m.filter->matches(elem, m.match_func);
	if (!ok) {
		return {name, false, false};
	}

	// skip must not match.
	// ignore partial match so we can get to more precise match later.
	auto [skip, partial_skip] = m.skip->matches(elem, m.match_func);
	if (skip && !partial_skip) {
		return {name, false, false};
	}

	return {name, ok, partial};
}

struct SimpleMatch : FilterMatch {
	std::vector<String> m;

	virtual Result matches(view<str> name, std::function<bool(str,str,error)> match_string) override {
		size i = -1;
		for (str s : name) {
			i++;
			if (i >= len(m)) {
				break;
			}
			if (bool ok = match_string(m[i], s, error::ignore); !ok) {
				return {false, false};
			}
		}
		return {true, len(name) < len(m)};
	}

	virtual void verify(str name, std::function<bool(str,str,error)> match_string, error err) override {
		size i = -1;
		for (str s : m) {
			i++;
			m[i] = rewrite(s);
		}
		// Verify filters before doing any processing.
		i = -1;
		for (str s : m) {
			i++;
			match_string(s, "non-empty", [&](Error &e) {
				err(fmt::errorf("element %d of %s (%q): %s", i, name, s, e));
			});
			if (err) {
				return;
			}
		}
		return;
	}
} ;

struct AlternationMatch : FilterMatch {
	std::vector<FilterMatch*> m;

	virtual Result matches(view<str> name, std::function<bool(str,str,error)> match_string) override {
		for (FilterMatch *m : m) {
			if (Result r = m->matches(name, match_string); r.ok) {
				return r;
			}
		}
		return {false, false};
	}

	virtual void verify(str name, std::function<bool(str,str,error)> match_string, error err) override {
		size i = -1;
		for (FilterMatch *m : m) {
			i++;
			m->verify(name, match_string, [&](Error &e) {
				err(fmt::errorf("alternation %d of %s", i, e));
			});
			if (err) {
				return;
			}
		}
		return;
	}
} ;

Matcher detail::new_matcher(std::function<bool(str pat, str s, error)> const &match_string, str patterns, str name, str skips) {
	// var filter, skip filterMatch
	std::unique_ptr<FilterMatch> filter, skip;
	if (patterns == "") {
		filter = std::make_unique<SimpleMatch>(); // always partial true
	} else {
		panic("unimplemented");
		// filter = splitRegexp(patterns)
		// if err := filter.verify(name, matchString); err != nil {
		// 	fmt.Fprintf(os.Stderr, "testing: invalid regexp for %s\n", err)
		// 	os.Exit(1)
		// }
	}
	if (skips == "") {
		skip = std::make_unique<AlternationMatch>(); // always false
	} else {
		panic("unimplemented");
		// skip = splitRegexp(skips)
		// if err := skip.verify("-test.skip", matchString); err != nil {
		// 	fmt.Fprintf(os.Stderr, "testing: invalid regexp for %v\n", err)
		// 	os.Exit(1)
		// }
	}
	return Matcher{
		.filter =    std::move(filter),
		.skip =      std::move(skip),
		.match_func = match_string,
		// .sub_names =  map[string]int32{},
	};
}

// parse_subtest_number splits a subtest name into a "#%02d"-formatted int32
// suffix (if present), and a prefix preceding that suffix (always).
static std::pair<String /*prefix*/, int32 /*nn*/> parse_subtest_number(str s) {
	size i = strings::last_index(s, "#");
	if (i < 0) {
		return {s, 0};
	}

	str prefix = s[0,i];
	str suffix = s+(i+1);
	if (len(suffix) < 2 || (len(suffix) > 2 && suffix[0] == '0')) {
		// Even if suffix is numeric, it is not a possible output of a "%02" format
		// string: it has either too few digits or too many leading zeroes.
		return {s, 0};
	}
	if (suffix == "00") {
		if (!strings::has_suffix(prefix, "/")) {
			// We only use "#00" as a suffix for subtests named with the empty
			// string — it isn't a valid suffix if the subtest name is non-empty.
			return {s, 0};
		}
	}

	bool ok = true;
	size n = strconv::parse_int(suffix, 10, 32, [&](Error&) {
		ok = false;
	});
	if (!ok || n < 0) {
		return {s, 0};
	}
	return {prefix, int32(n)};
}

String Matcher::unique(str parent, str subname) {
	Matcher &m = *this;
	String base = parent + "/" + subname;

	for (;;) {
		int32 n = m.sub_names[base];
		if (n < 0) {
			panic("subtest count overflow");
		}
		m.sub_names[base] = n + 1;

		if (n == 0 && subname != "") {
			auto [prefix, nn] = parse_subtest_number(base);
			if (len(prefix) < len(base) && nn < m.sub_names[prefix]) {
				// This test is explicitly named like "parent/subname#NN",
				// and #NN was already used for the NNth occurrence of "parent/subname".
				// Loop to add a disambiguating suffix.
				continue;
			}
			return base;
		}

		String name = fmt::sprintf("%s#%02d", base, n);
		if (m.sub_names[name] != 0) {
			// This is the nth occurrence of base, but the name "parent/subname#NN"
			// collides with the first occurrence of a subtest *explicitly* named
			// "parent/subname#NN". Try the next number.
			continue;
		}

		return name;
	}
}
