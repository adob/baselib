import lib.fmt;
import lib.testing;
import <boost/core/pointer_traits.hpp>;
import <boost/move/utility_core.hpp>;
import lib.sync.chan;
import lib.sync.once;
import lib.sync.go;

using namespace lib;
using namespace sync;

struct One {
    int i = 0;

    void increment() {
        i++;
    }
} ;

void run(testing::T &t, Once &once, One &o, Chan<bool> &c) {
    once.run([&]{ o.increment(); });

    if (int v = o.i; v != 1) {
        t.errorf("once failed inside run: %d is not 1", v);
    }

    c.send(true);
}

void test_once(testing::T &t) {
    One o;

    Once once;
    Chan<bool> c;

    const int N = 10;
    for (int i = 0; i < N; i++) {
        go([&] {
            run(t, once, o, c);
        });
    }

    for (int i = 0; i < N; i++) {
        c.recv();
    }

    if (o.i != 1) {
        t.errorf("once failed outside run: %s is not 1", o.i);
    }
}