#include "fmt.h"

#include "gtest/gtest.h"

using namespace lib;

TEST(fmt, sprintf) {
    EXPECT_EQ(fmt::sprintf("hello"), "hello");

    EXPECT_EQ(fmt::sprintf("%v", (byte) 42), str("42"));

    EXPECT_EQ(fmt::sprintf("%v", true), str("true"));
    EXPECT_EQ(fmt::sprintf("%v", false), str("false"));
}
