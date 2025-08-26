#pragma once

#include "lib/types.h"

#ifdef ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#else
#include <pthread.h>
#endif

namespace lib::sync {
    struct Mutex : noncopyable {

    #ifdef ESP_PLATFORM
        SemaphoreHandle_t mutex;
        StaticSemaphore_t data;
        Mutex();
        ~Mutex();
    #else
        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 
    #endif
        
        void lock();
        bool try_lock();
        void unlock();
    } ;
}
