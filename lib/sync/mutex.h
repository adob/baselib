#pragma once

#include "lib/types.h"
#include <pthread.h>

namespace lib::sync {
    struct Mutex : noncopyable {
        pthread_mutex_t mutex;
        
        Mutex();
        
        void lock();
        bool try_lock();
        void unlock();
        
        // ~Mutex();
    } ;
}
