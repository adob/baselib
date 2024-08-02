#pragma once

#include "mutex.h"

namespace lib::sync {
    struct Lock {
        Mutex& mutex;
        bool locked;
        
        Lock(Mutex&);
        void relock();
        void unlock();
        ~Lock();
    } ;


    struct TryLock {
        Mutex& mutex;
        bool locked;
        
        TryLock(Mutex&);
        operator bool() const;
        ~TryLock();
    } ;
}
