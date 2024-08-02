#include "mutex.h"
#ifndef ARDUINO

#include "lib/base.h"
#include "lib/os.h"

using namespace lib;
using namespace sync;

Mutex::Mutex() {
    pthread_mutexattr_t attr;
    
    if (int code = pthread_mutexattr_init(&attr)) {
        panic(os::from_errno(code));
    }

    if (int code = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE)) {
        pthread_mutexattr_destroy(&attr);
        panic(os::from_errno(code));
    }

    //         if (pthread_mutexattr_settype&attr, PTHREAD_MUTEX_DEFAULT) {
    //             pthread_mutexattr_destory(&attr);
    //             throw error(errno);
    //         }
    //         
    //         if (pthread_mutexattr_setprotocol&attr, PTHREAD_PRIO_NONE)) {
    //             pthread_mutexattr_destory(&attr);
    //             throw error(errno);
    //         }

    if (int code = pthread_mutex_init(&mutex, &attr)) {
        pthread_mutexattr_destroy(&attr);
        panic(os::from_errno(code));
    }

    if (int code = pthread_mutexattr_destroy(&attr)) {
        panic(os::from_errno(code));
    }
    //print "create";
}

void Mutex::lock() {
    //print "lock";
    //debug::print_backtrace();
    if (int code = pthread_mutex_lock(&mutex)) {
        panic(os::from_errno(code));
    }
        
}

bool Mutex::try_lock() {
    int r = pthread_mutex_trylock(&mutex);
    if (r == 0) {
        return true;
    }
    if (r == EBUSY) {
        return false;
    }
    panic(os::from_errno(r));
    return false;
}

void Mutex::unlock() {
    //print "unlock";
    if (int code = pthread_mutex_unlock(&mutex)) {
        panic(os::from_errno(code));
    }
}
    
// Mutex::~Mutex() {
//     //print "destroy";
//     if (int code = pthread_mutex_destroy(&mutex)) {
//         panic(os::from_errno(code));
//     }
// }
    
#endif