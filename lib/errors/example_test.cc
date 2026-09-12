import lib.base;
import lib.time;
import lib.fmt;

using namespace lib;

struct MyError : ErrorBase<MyError> {
    time::time when;
    String     what;

    // Format this error to out, forwarding write failures through err.
    void fmt(io::Writer &out, error err) const override {
        fmt::fprintf(out, err, "%v: %v", this->when, this->what);
    }
} ;

static MyError oops() {
    MyError e;
    e.when = time::date(1989, time::Month(3), 15, 22, 30, 0, 0, time::UTC);
    e.what = "the file system has gone away";
    return e;
}
