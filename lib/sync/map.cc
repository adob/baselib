export module lib.sync.map;
import lib.sync.lock;
import lib.sync.rwmutex;
import lib.types;

// TODO: Investigate Clang template visibility failures with plain imports;
// remove these re-exports if they are only compiler workarounds.
export import <unordered_map>;


export extern "C++" {
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
}
