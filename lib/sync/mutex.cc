export module lib.sync.mutex;

#ifdef TEENSYDUINO
    export import lib.sync.mutex_teensy;
#else
import lib.types;
#include "mutex_impl.h"

#ifdef ESP_PLATFORM
import <freertos/FreeRTOS.h>;
import <freertos/semphr.h>;
#elif AZURE_RTOS
import "tx_api.h";
#elif __ZEPHYR__
import <zephyr/kernel.h>;
#else
import <pthread.h>;
#endif


export extern "C++"
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
#endif