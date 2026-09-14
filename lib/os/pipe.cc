module;
#include "pipe_impl.h"

export module lib.os.pipe;
import lib.error;
import lib.os.file;


export extern "C++"
namespace lib::os {
    struct FilePair {
        File reader;
        File writer;
    };
    
    FilePair pipe(error err);
    FilePair pipe(int flags, error err);
}
