#include "cond.h"
#include "lib/os.h"

using namespace lib;
using namespace sync;

#ifndef ARDUINO

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

void Cond::wait(Mutex& mutex) {
    if (int code = pthread_cond_wait(&cond, &mutex.mutex)) {
        panic(os::Errno(code));
    }
}

void Cond::signal() {
    if (int code = pthread_cond_signal(&cond)) {
        panic(os::Errno(code));
    }
}

void Cond::broadcast() {
    if (int code = pthread_cond_broadcast(&cond)) {
        panic(os::Errno(code));
    }
}

// Cond::~Cond() {
//     if (int code = pthread_cond_destroy(&cond)) {
//         panic(os::from_errno(code));
//     }
// }
    
#endif