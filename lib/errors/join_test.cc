#include "lib/testing/testing.h"
import lib.base;
import "errors.h";
#include "join.h"
import lib.error;
import lib.exceptions;
#include <initializer_list>

using namespace lib;
using namespace errors;


void test_join(testing::T &t) {
    BasicError err1 = errors::create("err1");
    BasicError err2 = errors::create("err2");

    struct TestCase { 
        std::initializer_list<Error*> errs;
        std::initializer_list<Error*> want;
    } ;
    for (auto &test : (TestCase[]) {
        {
            .errs = {&err1},
            .want = {&err1}
        }, {
            .errs = {&err1, &err2},
            .want = {&err1, &err2},
        },
    }) {
        // Borrow the initializer-list arrays, which remain alive throughout this loop.
        view<Error*> want = test.want;
        view<Error*> got = errors::join(test.errs).unwrap();
        if (got != want) {
            t.errorf("join(%v) = %v; want %v", test.errs, got, test.want);
        }
    }
}
