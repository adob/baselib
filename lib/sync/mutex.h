#pragma once

#include "lib/types.h"

#ifdef ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#elif AZURE_RTOS
#include "tx_api.h"
#elif __ZEPHYR__
#include <zephyr/kernel.h>
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
    #elif AZURE_RTOS
        TX_MUTEX mutex;

        Mutex();
        ~Mutex();
    #elif __ZEPHYR__
        k_mutex mutex;

        Mutex();
    #else
        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 
    #endif
        
        void lock();
        bool try_lock();
        void unlock();
    } ;
}
