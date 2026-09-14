import <string>;
import lib;
import lib.array;
import lib.bitflag;
import lib.concepts;
import lib.exception;
import lib.fallback;
import lib.inline_string;
import lib.type_id;
import lib.utils;

import lib.testing;

// Exercise the public module imports together; t reports API or linkage failures.
void test_top_level_module_exports(lib::testing::T &t) {
    int calls = 0;
    {
        lib::defer cleanup([&] { ++calls; });
    }
    lib::InlineString<16> text("hello");
    lib::Array<int, 2> values{};
    values[0] = 7;
    lib::bitflag<unsigned> bits(3);
    static_assert(lib::concepts::Sizeable<std::string>);
    if (calls != 1 || text.length != 5 || values[0] != 7 ||
        unsigned(bits) != 3 || lib::type_id<int> == lib::type_id<double>) {
        t.errorf("top-level module exports must retain their behavior");
    }
}
