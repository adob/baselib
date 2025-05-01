#include "pipe.h"
#include <memory>

using namespace lib;
using namespace lib::io;
using namespace lib::io::internal;

PipePair io::pipe() {
    PipePair p {
        .r = std::make_shared<PipeReader>(),
        .w = std::make_shared<PipeWriter>(),
    };

    p.w->r = p.r;

    return p;
 };
 
 ReadResult PipeReader::direct_read(buf bytes, error err) {
    return this->pipe.read(bytes, err);
 }

 size PipeWriter::direct_write(str data, error err) {
    return this->r->pipe.write(data, err);
 }
 
 ReadResult internal::Pipe::read(buf bytes, error err) {
    Pipe &p = *this;

    int r = sync::poll(
        sync::Recv(p.done)
    );
    if (r == 0) {
        p.read_close_error(err);
        return {0, true};
    }

    str bw;
    r = sync::select(
        sync::Recv(p.wr_ch, &bw),
        sync::Recv(p.done)
    );

    if (r == 0) {
        size nr = copy(bytes, bw);
        p.rd_ch.send(nr);
        return {nr, false};
    }
    
    p.read_close_error(err);
    return {0, true};
 }

 void internal::Pipe::read_close_error(error err) {
    Pipe &p = *this;
    sync::Lock lock(p.err_mu);

    if (!p.rd_err) {
        if (p.wr_err) {
            err(p.wr_err.to_error());
            return;
        }

        if (p.wr_closed) {
            return;
        }
    }

    err(ErrClosedPipe());
 }

 size internal::Pipe::write(str data, error err) {
    Pipe &p = *this;

    int r =  sync::poll(
        sync::Recv(p.done)
    );
    if (r == 0) {
        p.write_close_error(err);
        return 0;
    }
    sync::Lock lock(p.wr_mu);
    size n = 0;
    
    for (bool once = true; once || len(data) > 0; once = false) {
        int r = sync::select(
            sync::Send(p.wr_ch, data),
            sync::Recv(p.done)
        );
        if (r == 0) {
            size nw = p.rd_ch.recv();
            data = data+nw;
            n += nw;
        } else {
            p.write_close_error(err);
            return n;
        }
    }

    return n;
 }

 void internal::Pipe::write_close_error(error err) {
    Pipe &p = *this;
    sync::Lock lock(p.err_mu);

    if (!p.wr_err && p.rd_err) {
        err(p.rd_err.to_error());
        return;
    }

    err(ErrClosedPipe());
 }


 void PipeReader::close(error) {
    return this->pipe.close_read(nil);
 }

 void PipeReader::close_with_error(Error const &e) {
    return this->pipe.close_read(&e);
 }

 void PipeWriter::close(error) {
    return this->r->pipe.close_write(nil);
 }

 void PipeWriter::close_with_error(Error const &e) {
    return this->r->pipe.close_write(&e);
 }

 void internal::Pipe::close_read(Error const *e) {
    Pipe &p = *this;
    sync::Lock lock(p.err_mu);

    if (e != nil) {
        p.rd_err.report(*e);
    }

    p.once.run([&] { p.done.close(); });
 }
 
 void internal::Pipe::close_write(Error const *e) {
    Pipe &p = *this;
    sync::Lock lock(p.err_mu);

    if (p.wr_err.has_error || p.wr_closed) {
        return;
    }

    if (e != nil) {
        p.wr_err.report(*e);
    } else {
        p.wr_closed = true;
    }

    p.once.run([&] { p.done.close(); });
 }
