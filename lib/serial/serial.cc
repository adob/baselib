module;
#include "serial_impl+linux.h"

export module lib.serial;
import lib.str;
import lib.error;
import lib.os.file;


export extern "C++" {
namespace lib::serial {
    struct Port : lib::os::File {} ;
    Port open(lib::str path, lib::error err);
}
}
