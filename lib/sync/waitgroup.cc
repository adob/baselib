module;
#include "waitgroup_impl.h"

export module lib.sync.waitgroup;
import lib.sync.mutex;
import lib.sync.cond;


export extern "C++"
namespace lib::sync {
    struct WaitGroup {
        int   cnt;
        Mutex mtx;
        Cond cond;


        explicit WaitGroup(int n = 0);
        void add(int);
        void done();
        void wait();
    };
}
