#include "rwmutex.h"
#include "lib/os/error.h"
#include <pthread.h>
using namespace lib;
using namespace sync;

// RWMutex::RWMutex() {}
void RWMutex::r_lock() {
#ifdef __ZEPHYR__
    panic("unimplemented");
#else
    int code = pthread_rwlock_rdlock(&this->rwlock);
    if (code != 0) {
        panic(os::Errno(code));
    }
#endif
}
bool lib::sync::RWMutex::try_r_lock() {
#ifdef __ZEPHYR__
    panic("unimplemented");
    return false;
#else
    int r = pthread_rwlock_tryrdlock(&this->rwlock);
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

void lib::sync::RWMutex::r_unlock() {
#ifdef __ZEPHYR__
    panic("unimplemented");
#else
    if (int code = pthread_rwlock_unlock(&this->rwlock)) {
        panic(os::Errno(code));
    }
#endif
}

void lib::sync::RWMutex::lock() {
#ifdef __ZEPHYR__
    panic("unimplemented");
#else
    int code = pthread_rwlock_wrlock(&this->rwlock);
    if (code != 0) {
        panic(os::Errno(code));
    }
#endif
}

bool lib::sync::RWMutex::try_lock() {
#ifdef __ZEPHYR__
    panic("unimplemented");
#else
    int r = pthread_rwlock_trywrlock(&this->rwlock);
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

void lib::sync::RWMutex::unlock() {
#ifdef __ZEPHYR__
    panic("unimplemented");
#else
    if (int code = pthread_rwlock_unlock(&this->rwlock)) {
        panic(os::Errno(code));
    }
#endif
}

// lib::sync::RWMutex::~RWMutex() {}
