#include "mutex.h"
#include "lib/os/error.h"

using namespace lib;
using namespace sync;

#ifdef ESP_PLATFORM
    Mutex::Mutex() {
        mutex = xSemaphoreCreateMutexStatic(&data);
    }

    Mutex::~Mutex() {
        vSemaphoreDelete(mutex);
    }
#endif


void Mutex::lock() {
#ifdef ESP_PLATFORM
    xSemaphoreTake(mutex, portMAX_DELAY);
#else
    if (int code = pthread_mutex_lock(&mutex)) {
        panic(os::Errno(code));
    }
#endif
    
}

bool Mutex::try_lock() {
#ifdef ESP_PLATFORM
    return xSemaphoreTake(mutex, 0) == pdTRUE;
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