#include "cond.h"
#include "lib/os.h"

using namespace lib;
using namespace sync;

#ifndef ESP_PLATFORM

// Cond::Cond() {
//     pthread_condattr_t attr;
//     
//     if (int code = pthread_condattr_init(&attr))
//          panic(os::from_errno(code));
//     
//     if (int code = pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE)) {
//         pthread_condattr_destroy(&attr);
//         panic(os::from_errno(code));
//     }
//     
//     if (int code = pthread_cond_init(&cond, &attr)) {
//         pthread_condattr_destroy(&attr);
//         panic(os::from_errno(code));
//     }
//     
//     if (int code = pthread_condattr_destroy(&attr)) {
//          panic(os::from_errno(code));
//     }
// }

#ifdef __ZEPHYR__
Cond::Cond() {
    k_condvar_init(&this->cond);
}
#endif

void Cond::wait(Mutex& mutex) {
#ifdef __ZEPHYR__
    k_condvar_wait(&this->cond, &mutex.mutex, K_FOREVER);
#else
    if (int code = pthread_cond_wait(&cond, &mutex.mutex)) {
        panic(os::Errno(code));
    }
#endif
}

void Cond::signal() {
#ifdef __ZEPHYR__
    k_condvar_signal(&this->cond);
#else
    if (int code = pthread_cond_signal(&cond)) {
        panic(os::Errno(code));
    }
#endif
}

void Cond::broadcast() {
#ifdef __ZEPHYR__
    k_condvar_broadcast(&this->cond);
#else
    if (int code = pthread_cond_broadcast(&cond)) {
        panic(os::Errno(code));
    }
#endif
}

// Cond::~Cond() {
//     if (int code = pthread_cond_destroy(&cond)) {
//         panic(os::from_errno(code));
//     }
// }
    
#endif