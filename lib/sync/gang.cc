#include "lib/sync/go.h"
#include "gang.h"

using namespace lib;
using namespace lib::sync;

void Gang::join() {
    for (sync::go &g : this->gs) {
        g.join();
    }
    this->gs.clear();
}

Gang::~Gang() {
    for (sync::go &g : this->gs) {
        g.join();
    }
}