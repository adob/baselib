#pragma once
#include <future>

namespace lib::sync {
    
    template<typename Function, typename... Args>
    auto go(Function&& f, Args&&... args ) {
        return std::async(std::launch::async, 
                          std::forward<Function>(f),
                          std::forward<Args>(args)...);
    }

}
