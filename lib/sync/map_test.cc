import lib.sync.map;
import lib.testing;

// Instantiate the exported map template; t checks insertion and deletion.
void test_map_store_delete(lib::testing::T &t) {
    lib::sync::Map<int, int> map;
    map.store(7, 42);
    if (map.data.at(7) != 42) {
        t.error("map did not store its value");
    }
    map.del(7);
    if (!map.data.empty()) {
        t.error("map did not delete its value");
    }
}
