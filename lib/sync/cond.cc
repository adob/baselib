module;
#include "cond_impl.h"

export module lib.sync.cond;
import lib.sync.mutex;

import lib.types;
#ifdef __ZEPHYR__
import <zephyr/kernel.h>;
#else
import <pthread.h>;
#endif



export extern "C++"
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
