import lib.async;
import lib.testing;

// Instantiate the exported future API in a consumer; t reports the result.
void test_future_result(lib::testing::T &t) {
    auto future = lib::async::go([] { return 42; });
    if (future.await() != 42) {
        t.error("async future returned the wrong result");
    }
}
