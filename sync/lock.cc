#include "lock.h"

#include "lib/base.h"

using namespace lib;
using namespace sync;

Lock::Lock(Mutex& mutex) : mutex(mutex), locked(true) {
    mutex.lock();
}

void Lock::relock() {
    if (locked)
        panic("sync::Lock: already locked");
    locked = true;
    mutex.lock();
}

void Lock::unlock() {
    if (!locked)
        panic("sync::Lock: already unlocked");
    
    mutex.unlock();
    locked = false;
}

Lock::~Lock() {
    if (locked)
        mutex.unlock();
}

TryLock::TryLock(Mutex& mutex) : mutex(mutex) {
    locked = mutex.try_lock();
}

TryLock::operator bool() const {
    return locked;
}

TryLock::~TryLock() {
    if (locked) {
        mutex.unlock();
    }
}
