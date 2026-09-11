#pragma once

#include "lib/error.h"
#include "file.h"

namespace lib::os {
    struct FilePair {
        File reader;
        File writer;
    };
    
    FilePair pipe(error err);
    FilePair pipe(int flags, error err);
}
