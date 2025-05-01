#pragma once

#include "lib/error.h"
#include "lib/io/io.h"
#include "lib/sync/chan.h"
#include "lib/sync/mutex.h"
#include "lib/sync/once.h"

#include <memory>

namespace lib::io {
    namespace internal {
        struct Pipe {
            sync::Mutex      wr_mu;  // Serializes write operations
            sync::Chan<str>  wr_ch;
            sync::Chan<size> rd_ch;

            sync::Once once;
            sync::Chan<void> done;

            sync::Mutex err_mu;
            ErrorRecorder wr_err;
            ErrorRecorder rd_err;
            bool wr_closed = false;

            ReadResult read(buf bytes, error err);
            size write(str data, error err);
            void read_close_error(error err);
            void write_close_error(error err);

            void close_read(Error const *e);
            void close_write(Error const *e);
        } ;
    };
    struct PipePair;
    PipePair pipe();
    struct PipeWriter;

    struct ErrClosedPipe : ErrorBase<ErrClosedPipe, "io: read/write on closed pipe"> {};

    struct PipeReader : io::Reader {
        // Read implements the standard Read interface:
        // it reads data from the pipe, blocking until a writer
        // arrives or the write end is closed.
        // If the write end is closed with an error, that error is
        // returned as err; otherwise err is EOF.
        ReadResult direct_read(buf bytes, error err) override;
        void close(error) override;

        void close_with_error(Error const &e);

      private:
        internal::Pipe pipe;
        friend PipeWriter;
    } ;

    struct PipeWriter : io::Writer {
      size direct_write(str data, error err) override;
      void close(error) override;

      void close_with_error(Error const &e);

    private:
        std::shared_ptr<PipeReader> r;
        friend PipePair pipe();
    } ;

    struct PipePair {
        std::shared_ptr<PipeReader> r;
        std::shared_ptr<PipeWriter> w;
    } ;

    // pipe creates a synchronous in-memory pipe.
    // It can be used to connect code expecting an [io.Reader]
    // with code expecting an [io.Writer].
    //
    // Reads and Writes on the pipe are matched one to one
    // except when multiple Reads are needed to consume a single Write.
    // That is, each Write to the [PipeWriter] blocks until it has satisfied
    // one or more Reads from the [PipeReader] that fully consume
    // the written data.
    // The data is copied directly from the Write to the corresponding
    // Read (or Reads); there is no internal buffering.
    //
    // It is safe to call Read and Write in parallel with each other or with Close.
    // Parallel calls to Read and parallel calls to Write are also safe:
    // the individual calls will be gated sequentially.
    PipePair pipe();
}