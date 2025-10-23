#pragma once

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#else
#include <pthread.h>
#endif

#include "mutex.h"
#include "lib/base.h"

namespace lib::sync {
    struct Cond : noncopyable {
    #ifdef __ZEPHYR__
        k_condvar cond;
        Cond();
    #else
        pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    #endif
        
        //Cond() {}
        void wait(Mutex&);
        void signal();
        void broadcast();
        //~Cond();
    } ;
}
