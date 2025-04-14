#include "rwmutex.h"
#include "lib/os/error.h"
#include <pthread.h>
using namespace lib;
using namespace sync;

// RWMutex::RWMutex() {}
void RWMutex::r_lock() {
    int code = pthread_rwlock_rdlock(&this->rwlock);
    if (code != 0) {
        panic(os::Errno(code));
    }
}
bool lib::sync::RWMutex::try_r_lock() {
    int r = pthread_rwlock_tryrdlock(&this->rwlock);
    if (r == 0) {
        return true;
    }
    if (r == EBUSY) {
        return false;
    }
    panic(os::Errno(r));
    return false;
}

void lib::sync::RWMutex::r_unlock() {
    if (int code = pthread_rwlock_unlock(&this->rwlock)) {
        panic(os::Errno(code));
    }
}

void lib::sync::RWMutex::lock() {
    int code = pthread_rwlock_wrlock(&this->rwlock);
    if (code != 0) {
        panic(os::Errno(code));
    }
}

bool lib::sync::RWMutex::try_lock() {
    int r = pthread_rwlock_trywrlock(&this->rwlock);
    if (r == 0) {
        return true;
    }
    if (r == EBUSY) {
        return false;
    }
    panic(os::Errno(r));
    return false;
}

void lib::sync::RWMutex::unlock() {
    if (int code = pthread_rwlock_unlock(&this->rwlock)) {
        panic(os::Errno(code));
    }
}

// lib::sync::RWMutex::~RWMutex() {}
