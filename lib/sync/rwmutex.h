#pragma once

#include "lib/sync/mutex.h"
#include "lib/types.h"

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#else
#include <pthread.h>
#endif

namespace lib::sync {
    struct RWMutex : noncopyable {
    #ifdef __ZEPHYR__
    #else
        pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
    #endif
    
        // Mutex w;
        // uint32 writer_sem = 0;
        // uint32 reader_sem = 0;
        // std::atomic<int32> reader_count = 0;
        // std::atomic<int32> reader_wait = 0;

        // RWMutex();

        void r_lock();
        bool try_r_lock();
        void r_unlock();
        void lock();
        bool try_lock();
        void unlock();

        // ~RWMutex();
    } ;
}
