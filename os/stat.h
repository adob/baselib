#pragma once

#include "lib/base.h"


namespace lib::os {
    struct FileInfo { 
        int64 size;
    } ;

    FileInfo stat(str path, error&);
}