#include "waitgroup.h"
#include "lock.h"

using namespace lib;
using namespace lib::sync;

WaitGroup::WaitGroup(int n) : cnt(n) {}

void WaitGroup::add(int n) {
    Lock lock(mtx);
    cnt += n;

    if (cnt == 0) {
        cond.signal();
    }
}

void WaitGroup::done() {
    add(-1);
}

void WaitGroup::wait() {
    Lock lock(mtx);
    if (cnt != 0) {
        cond.wait(mtx);
    }
}