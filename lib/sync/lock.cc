module;
#include "lock_impl.h"

export module lib.sync.lock;
import lib.sync.mutex;


import lib.types;
#if !defined(ESP_PLATFORM) && !defined(AZURE_RTOS)
import lib.sync.rwmutex;
#endif


export extern "C++" {
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

    #if !defined(ESP_PLATFORM) && !defined(AZURE_RTOS)
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

    #endif

    // struct TryLock {
    //     Mutex& mutex;
    //     bool locked;
        
    //     TryLock(Mutex&);
    //     operator bool() const;
    //     ~TryLock();
    // } ;
}

}
