#pragma once

#include <pthread.h>
#include "mutex.h"
#include "lib/base.h"

namespace lib::sync {
    struct Cond : noncopyable {
        pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
        
        //Cond() {}
        void wait(Mutex&);
        void signal();
        void broadcast();
        //~Cond();
    } ;
}
