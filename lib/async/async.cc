export module lib.async;


import <utility>;

// TODO: Investigate Clang template visibility failures with plain imports;
// remove these re-exports if they are only compiler workarounds.
export import <future>;

export extern "C++" {
namespace lib::async {
    template <typename T>
    struct Future {
        std::future<T> future;

        T await() {
            return future.get();
        }

        // ~Future() {
        //     future.wait_for(std::chrono::seconds(0));
        // }
    };

    template <typename Function, typename... Args>
    auto
    go(Function &&f, Args &&...args) {
        return Future { std::async(std::launch::async, std::forward<Function>(f), std::forward<Args>(args)...) };
    }
}
}
