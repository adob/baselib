#pragma once

#include "rwmutex.h"
#include "mutex.h"

namespace lib::sync {
    struct Lock {
        Mutex *mutex = nil;
        bool locked = false;
        
        Lock() {};
        Lock(Mutex&);
        Lock(Mutex*);

        void lock(Mutex&);
        bool try_lock(Mutex&);

        void relock();
        bool try_relock();
        void unlock();
        ~Lock();
    } ;

    struct RLock {
        RWMutex *mutex = nil;
        bool locked = false;
        
        RLock() {};
        RLock(RWMutex&);
        RLock(RWMutex*);

        void lock(RWMutex&);
        bool try_lock(RWMutex&);

        void relock();
        bool try_relock();
        void unlock();
        ~RLock();
    } ;

    struct WLock {
        RWMutex *mutex = nil;
        bool locked = false;
        
        WLock() {};
        WLock(RWMutex&);
        WLock(RWMutex*);

        void lock(RWMutex&);
        bool try_lock(RWMutex&);

        void relock();
        bool try_relock();
        void unlock();
        ~WLock();
    } ;


    // struct TryLock {
    //     Mutex& mutex;
    //     bool locked;
        
    //     TryLock(Mutex&);
    //     operator bool() const;
    //     ~TryLock();
    // } ;
}
