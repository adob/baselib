#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <unistd.h>
#include <errno.h>

#include "pipe.h"
#include "error.h"

using namespace lib;

os::FilePair os::pipe(error err) {
    return pipe(O_CLOEXEC, err);
}

os::FilePair os::pipe(int flags, error err) {
    int pipefds[2];
    
    int r = ::pipe2(pipefds, flags);
    if (r != 0) {
        err(SyscallError("pipe2", errno));
        return {};
    }
    
    FilePair pair {
        .reader  = File(pipefds[0]),
        .writer  = File(pipefds[1]),
    };
    
    pair.reader.resize_readbuf(4096);
    pair.writer.resize_writebuf(4096);
    
    return pair;
}
