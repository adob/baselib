#pragma once

#include "mutex.h"
#include "cond.h"

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