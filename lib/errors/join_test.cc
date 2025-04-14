#include "../testing.h"
#include "../base.h"
#include "./errors.h"
#include "./join.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/print.h"
#include <initializer_list>

using namespace lib;
using namespace errors;


template<class _Ep>
class initializer_list
{
    const _Ep* __begin_;
    size_t    __size_;
    
    inline
    constexpr
    initializer_list(const _Ep* __b, size_t __s) noexcept
        : __begin_(__b),
          __size_(__s)
    {
        print "created init list";
    }
public:
    typedef _Ep        value_type;
    typedef const _Ep& reference;
    typedef const _Ep& const_reference;
    typedef size_t    size_type;
    
    typedef const _Ep* iterator;
    typedef const _Ep* const_iterator;
    
    inline
    constexpr
    initializer_list() noexcept : __begin_(nullptr), __size_(0) {}
    
    inline
    constexpr
    size_t    size()  const noexcept {return __size_;}
    
    inline
    constexpr
    const _Ep* begin() const noexcept {return __begin_;}
    
    inline
    constexpr
    const _Ep* end()   const noexcept {return __begin_ + __size_;}
};


struct T {
    int i;
    T(int i) : i(i) {
        print "created", i;
    }

    ~T() {
        print "destroyed", i;
    }
} ;

struct S {
    std::initializer_list<T> list;
} ;

void f(std::initializer_list<T>) {

}

struct InitList {
    std::initializer_list<T> list;
};

void f2(initializer_list<int>) {

}


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
        view<Error*> got = errors::join(test.errs).unwrap();
        if (got != view(test.want)) {
            t.errorf("join(%v) = %v; want %v", test.errs, got, test.want);
        }
    }
}