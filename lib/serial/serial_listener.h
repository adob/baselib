#pragma once

#include "lib/io/io.h"
#include "lib/sync/mutex.h"

namespace serial {
    using namespace lib;

    struct Conn : io::StaticBuffered<512, 512> {
        sync::Mutex write_mtx;
    } ;

    struct Listener {
        virtual Conn& accept(error) = 0;
        virtual ~Listener() {}
    } ;
}