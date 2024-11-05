#include "error.h"
#include "lib/fmt/fmt.h"
#include "panic.h"
#include "fmt.h"
#include "io.h"

using namespace lib;

// error::Ignore error::ignore;

error::Panic error::panic;
error::Log error::log;

const deferror lib::ErrUnspecified("unspecified error");

void error::Panic::operator() (deferror const &err) {
    if (&err == &io::EOF) {
        this->ref = &err;
        return;
    }
    lib::panic(err.msg);
}

void error::Panic::operator() (str msg) {
    lib::panic(msg);
}

void error::Log::operator() (deferror const &err) {
    fmt::fprintf(stderr, "error: %s\n", err.msg);
}

void error::Log::operator() (str msg) {
    fmt::fprintf(stderr, "error: %s\n", msg);
}

void error::IgnoreOneImpl::operator() (deferror const &err) {
    if (&err == this->ignore) {
        parent.ref = &err;
        return;
    }
    parent(err);
}

void error::IgnoreOneImpl::operator() (str msg) {
    parent(msg);
}

error::IgnoreAll error::ignore() {
    return IgnoreAll();
}

error::IgnoreOne error::ignore(deferror const& err) {
    return IgnoreOne(*this, &err);
}


void error::fmt(fmt::Fmt& out) const {
    if (!ref) {
        out << "ok";
    } else {
        out << ref->msg;
    }
}

bool error::handle(deferror const& err) {
    if (ref == &err) {
        ref = nil;
        return true;
    }

    return false;
}