#pragma  once

#include "lib/sync/lock.h"
#include "lib/sync/rwmutex.h"
#include "lib/types.h"
#include <unordered_map>

namespace lib::sync {
    template <typename K, typename V>
    struct Map : noncopyable {
        RWMutex mu;
        std::unordered_map<K, V> data;

        // del deletes the value for a key.
        // If the key is not in the map, Delete does nothing.        
        void del(K const &k) {
            sync::WLock lock(mu);
            data.erase(k);
        }

        
        void store(K const &k, V const &v) {
            sync::WLock lock(mu);
            data[k] = v;
        }
    } ;
}