#pragma  once

#include "lib/os/file.h"

namespace lib::serial {
    struct Port : lib::os::File {} ;
    Port open(lib::str path, lib::error err);
}