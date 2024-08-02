#pragma once

#include <pthread.h>

namespace lib::sync {
    struct Mutex {
        pthread_mutex_t mutex;
        
        Mutex();
        Mutex(const Mutex&) = delete;
        void lock();
        bool try_lock();
        void unlock();
        
        Mutex& operator=(Mutex&) = delete;
        // ~Mutex();
    } ;
}
