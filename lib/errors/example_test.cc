#include "../base.h"
#include "../time.h"
#include "lib/fmt/fmt.h"

using namespace lib;

struct MyError : ErrorBase<MyError> {
    time::time when;
    String     what;

    virtual void describe(io::OStream &out) const override {
        fmt::fprintf(out, "%v: %v", this->when, this->what);
    }
} ;

static MyError oops() {
    MyError e;
    e.when = time::date(1989, time::Month(3), 15, 22, 30, 0, 0, time::UTC);
    e.what = "the file system has gone away";
    return e;
}

