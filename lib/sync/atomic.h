
#pragma once

#include "lib/sync/lock.h"
#include "lib/sync/mutex.h"
namespace lib::sync {
    enum MemoryOrder {
        Relaxed = __ATOMIC_RELAXED,
        Consume = __ATOMIC_CONSUME,
        Acquire = __ATOMIC_ACQUIRE,
        Release = __ATOMIC_RELEASE,
        AcqRel = __ATOMIC_ACQ_REL,
        SeqCst = __ATOMIC_SEQ_CST,
    } ;

    template <typename T>
    struct atomic {
        T value = 0;
        // mutable sync::Mutex mtx;

        atomic() : value(0) {}
        atomic(T val) : value(val) {}
        
        atomic<T> &operator=(atomic<T> const &other) = delete;

        constexpr static bool DebugLog = false;

        T add(T delta, MemoryOrder order = SeqCst) {
            // Lock l(mtx);
            T out = __atomic_add_fetch(&value, delta, order);
            if constexpr (DebugLog) {
                printf("%lu atomic add %d + %d -> %d\n", pthread_self(), value, delta, out);
            }
            return out;
        }

        T load(MemoryOrder order = Acquire) const {
            // Lock l(mtx);
            T out =  __atomic_load_n(&value, order);
            if constexpr (DebugLog) {
                printf("%u atomic load %d\n", pthread_self(), out);
            }
            return out;
        }

        void store(T newval, MemoryOrder order = Release) {
            // Lock l(mtx);
            __atomic_store_n(&value, newval, order);
            if constexpr (DebugLog) {
                printf("%u atomic store %d\n", pthread_self(), newval);
            }
        }

        bool compare_and_swap(T *oldval, T newval, 
                MemoryOrder success_meemorder = Release,
                MemoryOrder failure_memorder = Acquire
            ) {
            // Lock l(mtx);
            bool b = __atomic_compare_exchange_n(&value, oldval, newval, false, success_meemorder, failure_memorder);
            if constexpr (DebugLog) {
                printf("%u atomic compare-and-swap %d -> %d; ok %d\n", pthread_self(), *oldval, newval, b);
            }
            return b;
        }
    } ;

    template <typename T>
    T load(T const &t, MemoryOrder order = Acquire) {
        return __atomic_load_n(&t, order);
    }

    template <typename T>
    void store(T &t, T val, MemoryOrder order = Release) {
        __atomic_store_n(&t, val, order);
    }

    template <typename T>
    bool compare_and_swap(T &ptr, T *oldval, T newval, 
        MemoryOrder success_meemorder = Release,
        MemoryOrder failure_memorder = Acquire
    ) {
        bool b = __atomic_compare_exchange_n(&ptr, oldval, newval, false, success_meemorder, failure_memorder);
        return b;
    }
}