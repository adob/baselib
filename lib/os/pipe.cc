module;
#include "pipe_impl.h"

export module lib.os.pipe;
export import lib.error;
export import lib.os.file;


export extern "C++" {
namespace lib::os {
    struct FilePair {
        File reader;
        File writer;
    };
    
    FilePair pipe(error err);
    FilePair pipe(int flags, error err);
}

}
