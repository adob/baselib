#include "str.h"
#include "lib/testing/testing.h"

#include <algorithm>
#include <utility>
#include <vector>

using namespace lib;

// Report ownership-transfer or moved-from reuse failures through t.
void test_string_move_construction(testing::T &t) {
    String source("hello");
    byte *allocation = source.buffer.data;
    String destination(std::move(source));
    if (destination != "hello" || destination.buffer.data != allocation ||
        source.buffer.data != nil || source.buffer.len != 0 || source.length != 0) {
        t.errorf("move construction must transfer ownership and empty the source");
    }
    source = "reused";
    if (source != "reused" || destination != "hello") {
        t.errorf("a moved-from string must remain reusable");
    }
}

// Report failures when replacing allocations, assigning empty values, or self-moving through t.
void test_string_and_buffer_move_assignment(testing::T &t) {
    Buffer source(8);
    source.data[0] = 42;
    byte *allocation = source.data;
    Buffer destination(16);
    destination = std::move(source);
    if (destination.data != allocation || destination.len != 8 ||
        destination.data[0] != 42 || source.data != nil || source.len != 0) {
        t.errorf("buffer move assignment must transfer ownership and empty the source");
    }
    Buffer *buffer_alias = &destination;
    destination = std::move(*buffer_alias);
    if (destination.data != allocation || destination.len != 8) {
        t.errorf("buffer self-move must preserve its allocation");
    }

    String from("replacement");
    allocation = from.buffer.data;
    String to("old allocation");
    to = std::move(from);
    if (to != "replacement" || to.buffer.data != allocation || from.length != 0 ||
        from.buffer.data != nil || from.buffer.len != 0) {
        t.errorf("string move assignment must transfer ownership and empty the source");
    }
    String *string_alias = &to;
    to = std::move(*string_alias);
    if (to != "replacement" || to.buffer.data != allocation) {
        t.errorf("string self-move must preserve its value and allocation");
    }
    to = String();
    if (to.length != 0 || to.buffer.data != nil || to.buffer.len != 0) {
        t.errorf("moving an empty string must release the destination allocation");
    }
}

// Exercise the standard-library sorting and relocation paths that exposed the warning; t reports failures.
void test_string_moves_in_standard_algorithms(testing::T &t) {
    std::vector<String> values;
    for (str text : {str("delta"), str("alpha"), str("charlie"), str("bravo")}) {
        values.emplace_back(text);
    }
    std::partial_sort(values.begin(), values.begin() + 2, values.end());
    if (values[0] != "alpha" || values[1] != "bravo") {
        t.errorf("partial_sort must preserve string values");
    }
    std::sort(values.begin(), values.end());
    const str expected[] = {"alpha", "bravo", "charlie", "delta"};
    for (unsigned i = 0; i < 4; ++i) {
        if (values[i] != expected[i]) {
            t.errorf("sort must preserve every string value");
        }
    }
    byte *allocation = values[0].buffer.data;
    values.reserve(values.capacity() + 1);
    if (values[0] != "alpha" || values[0].buffer.data != allocation) {
        t.errorf("vector relocation should transfer string allocations without copying");
    }
}
