#pragma once

#include "lib/base.h"
#include "lib/io.h"
#include "file.h"

namespace lib::os {
    struct FilePair {
        File reader;
        File writer;
    };
    
    FilePair pipe(error2 &err);
    FilePair pipe(int flags, error2 &err);
}
