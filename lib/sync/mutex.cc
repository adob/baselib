#include "mutex.h"
#include "lib/base.h"

#ifdef ESP_PLATFORM
#elif AZURE_RTOS
#else
#include "lib/os/error.h"
#endif

using namespace lib;
using namespace sync;

#ifdef ESP_PLATFORM
    Mutex::Mutex() {
        mutex = xSemaphoreCreateMutexStatic(&data);
    }

    Mutex::~Mutex() {
        vSemaphoreDelete(mutex);
    }
#elif AZURE_RTOS
Mutex::Mutex() {
        uint status = tx_mutex_create(&this->mutex, nil, TX_INHERIT);
        if (status != TX_SUCCESS) {
            printf("tx_mutex_create: %u\n", status);
            panic("tx_mutex_create");
        }
    }

    Mutex::~Mutex() {
        uint status = tx_mutex_delete(&this->mutex);
        if (status != TX_SUCCESS) {
            printf("tx_mutex_delete: %u\n", status);
            panic("tx_mutex_delete");
        }
    }
#elif __ZEPHYR__
Mutex::Mutex() {
    k_mutex_init(&this->mutex);
}
#endif

void Mutex::lock() {
#ifdef ESP_PLATFORM
    xSemaphoreTake(mutex, portMAX_DELAY);
#elif AZURE_RTOS
    uint status = tx_mutex_get(&this->mutex, TX_WAIT_FOREVER);
    if (status != TX_SUCCESS) {
        printf("tx_mutex_get: %u\n", status);
        panic("tx_mutex_get");
    }
#elif __ZEPHYR__
    k_mutex_lock(&this->mutex, K_FOREVER);
#else
    if (int code = pthread_mutex_lock(&mutex)) {
        panic(os::Errno(code));
    }
#endif
    
}

bool Mutex::try_lock() {
#ifdef ESP_PLATFORM
    return xSemaphoreTake(mutex, 0) == pdTRUE;
#elif AZURE_RTOS
    uint status = tx_mutex_get(&this->mutex, TX_NO_WAIT);
    if (status == TX_SUCCESS) {
        return true;
    }
    if (status == TX_NOT_AVAILABLE) {
        return false;
    }
    
    printf("tx_mutex_get: %u\n", status);
    panic("tx_mutex_get");
    return false;
#elif __ZEPHYR__
    int ret = k_mutex_lock(&this->mutex, K_NO_WAIT);
    return ret == 0;
#else
    int r = pthread_mutex_trylock(&mutex);
    if (r == 0) {
        return true;
    }
    if (r == EBUSY) {
        return false;
    }
    panic(os::Errno(r));
    return false;
#endif
}

void Mutex::unlock() {
#ifdef ESP_PLATFORM
    if (xSemaphoreGive(mutex) == pdFALSE) {
        panic("xSemaphoreGive");
    }
#elif AZURE_RTOS
    uint status = tx_mutex_put(&this->mutex);
    if (status != TX_SUCCESS) {
        printf("tx_mutex_put: %u\n", status);
        panic("tx_mutex_put");
    }
#elif __ZEPHYR__
    int ret = k_mutex_unlock(&this->mutex);
    if (ret) {
        panic(os::Errno(ret));
    }
#else   
    if (int code = pthread_mutex_unlock(&mutex)) {
        panic(os::Errno(code));
    }
#endif
}
    
// Mutex::~Mutex() {
//     //print "destroy";
//     if (int code = pthread_mutex_destroy(&mutex)) {
//         panic(os::from_errno(code));
//     }
// }