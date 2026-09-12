module;
#include "gang_impl.h"

export module lib.sync.gang;
export import lib.sync.mutex;
export import lib.sync.go;
export import lib.sync.lock;
import <utility>;

// TODO: Investigate Clang template visibility failures with plain imports;
// remove these re-exports if they are only compiler workarounds.
export import <deque>;


export extern "C++" {
namespace lib::sync {
    struct Gang {
        Mutex mtx;
        std::deque<sync::go> gs;

        template<typename Function, typename... Args>
        void go(Function&& f, Args&&... args) {
            Lock lock(mtx);
            // new sync::go(std::forward<Function>(f), std::forward<Args>(args)...);
            this->gs.emplace_back(std::forward<Function>(f), std::forward<Args>(args)...);
        }

        void join();
        ~Gang();
    } ;
}
}
