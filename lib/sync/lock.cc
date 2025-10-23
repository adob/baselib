#include "lock.h"

#include "lib/base.h"

using namespace lib;
using namespace sync;

Lock::Lock(Mutex *mtx) : mutex(mtx) {
    if (mtx) {
        mutex->lock();
        this->locked = true;
    }
}

Lock::Lock(Mutex &mutex) : mutex(&mutex), locked(true) {
    mutex.lock();
}

void Lock::relock() {
    if (locked) {
        panic("sync::Lock::relock(): already locked");
    }
    locked = true;
    mutex->lock();
}

bool Lock::try_relock() {
    if (locked) {
        panic("sync::Lock::relock(): already locked");
    }
    this->locked = this->mutex->try_lock();
    return this->locked;
}

void Lock::lock(Mutex &mtx) {
    if (locked) {
        panic("sync::Lock::lock() already locked");
    }

    this->mutex = &mtx;
    this->locked = true;
    this->mutex->lock();
}

bool Lock::try_lock(Mutex &mtx) {
    if (locked) {
        panic("sync::Lock::lock() already locked");
    }

    this->mutex = &mtx;
    this->locked = this->mutex->try_lock();
    return this->locked;
}

void Lock::unlock() {
    if (!locked)
        panic("sync::Lock: already unlocked");
    
    mutex->unlock();
    locked = false;
}

Lock::~Lock() {
    if (locked) {
        mutex->unlock();
    }
}

#if !defined(ESP_PLATFORM) && !defined(AZURE_RTOS)
// RLock
RLock::RLock(RWMutex *mtx) : mutex(mtx) {
    if (mtx) {
        mutex->r_lock();
        this->locked = true;
    }
}

RLock::RLock(RWMutex &mutex) : mutex(&mutex), locked(true) {
    mutex.r_lock();
}

void RLock::relock() {
    if (locked) {
        panic("sync::RLock::relock(): already locked");
    }
    locked = true;
    mutex->r_lock();
}

bool RLock::try_relock() {
    if (locked) {
        panic("sync::RLock::relock(): already locked");
    }
    this->locked = this->mutex->try_r_lock();
    return this->locked;
}

void RLock::lock(RWMutex &mtx) {
    if (locked) {
        panic("sync::RLock::lock() already locked");
    }

    this->mutex = &mtx;
    this->locked = true;
    this->mutex->r_lock();
}

bool RLock::try_lock(RWMutex &mtx) {
    if (locked) {
        panic("sync::RLock::lock() already locked");
    }

    this->mutex = &mtx;
    this->locked = this->mutex->try_r_lock();
    return this->locked;
}

void RLock::unlock() {
    if (!locked)
        panic("sync::RLock: already unlocked");
    
    mutex->unlock();
    locked = false;
}

RLock::~RLock() {
    if (locked) {
        mutex->unlock();
    }
}

// WLock
WLock::WLock(RWMutex *mtx) : mutex(mtx) {
    if (mtx) {
        mutex->lock();
        this->locked = true;
    }
}

WLock::WLock(RWMutex &mutex) : mutex(&mutex), locked(true) {
    mutex.lock();
}

void WLock::relock() {
    if (locked) {
        panic("sync::WLock::relock(): already locked");
    }
    locked = true;
    mutex->lock();
}

bool WLock::try_relock() {
    if (locked) {
        panic("sync::WLock::relock(): already locked");
    }
    this->locked = this->mutex->try_lock();
    return this->locked;
}

void WLock::lock(RWMutex &mtx) {
    if (locked) {
        panic("sync::WLock::lock() already locked");
    }

    this->mutex = &mtx;
    this->locked = true;
    this->mutex->lock();
}

bool WLock::try_lock(RWMutex &mtx) {
    if (locked) {
        panic("sync::WLock::lock() already locked");
    }

    this->mutex = &mtx;
    this->locked = this->mutex->try_lock();
    return this->locked;
}

void WLock::unlock() {
    if (!locked)
        panic("sync::WLock: already unlocked");
    
    mutex->unlock();
    locked = false;
}

WLock::~WLock() {
    if (locked) {
        mutex->unlock();
    }
}
#endif
// TryLock::TryLock(Mutex& mutex) : mutex(mutex) {
//     locked = mutex.try_lock();
// }

// TryLock::operator bool() const {
//     return locked;
// }

// TryLock::~TryLock() {
//     if (locked) {
//         mutex.unlock();
//     }
// }
