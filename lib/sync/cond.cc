module;
#if !defined(TEENSYDUINO) && !defined(__ZEPHYR__)
#include <pthread.h>
#endif

export module lib.sync.cond;

#ifdef TEENSYDUINO
    export import :teensy;
#else
import lib;
import lib.sync.mutex;
#include "cond_impl.h"

#ifdef __ZEPHYR__
import <zephyr/kernel.h>;
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
#endif
