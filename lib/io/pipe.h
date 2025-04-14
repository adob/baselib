#pragma once

#include "lib/io/io_stream.h"
#include "lib/sync/chan.h"
#include "lib/sync/mutex.h"

namespace lib::io {
    namespace internal {
        struct Pipe {
            sync::Mutex      wr_mu;  // Serializes write operations
            sync::Chan<buf>  wr_ch;
            sync::Chan<size> rd_ch;
            sync::Chan<void> done;

            //sync::Once once;
            //'sync::Chan<void> done;
        } ;
    } ;

    struct PipeReader : io::Reader {
        size       direct_write(str data, error err) override;
    } ;

    struct PipeWriter : io::Writer {
        ReadResult direct_read(buf bytes, error err) override;

    } ;

    struct PipePair {
        PipeReader reader;
        PipeWriter writer;
    } ;

    PipePair pipe();
}