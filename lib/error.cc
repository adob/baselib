#include "error.h"
//#include "lib/fmt/fmt.h"
#include "lib/fmt/fmt.h"
#include "panic.h"
#include "fmt.h"
#include "io.h"

using namespace lib;

// static void test(error e) {

// }

// static void f() {
//     test(ErrorReporter([](const Error&) {}));
//     ErrorReporter erep = [](const Error &) {};
//     test(erep);
// }

//const ErrorReporter error::ignoring_error_reporter = [](const Error &) {};

namespace {
    
    // ErrorReporter panicking_error_reporter = [](const Error &e) { lib::panic(e); };
    // ErrorReporter logging_error_reporter = [](const Error &e) { errors::log_error(e); };

    void ignore_handler(const Error &) {}
    void panicking_handler(const Error &e) {
        panic(e);
    }
    void logging_handler(const Error &e) {
        errors::log_error(e);
    }
}


IgnoringError::IgnoringError() :
        error(error_reporter),
        error_reporter(ignore_handler) {}

PanickingError::PanickingError() : 
    error(error_reporter),
    error_reporter(panicking_handler) {}

LoggingError::LoggingError() : 
    error(error_reporter),
    error_reporter(logging_handler) {}

using Ignore = decltype(error::ignore);
using Panic = decltype(error::panic);
using Log = decltype(error::log);

Ignore::operator IgnoringError() {
    return IgnoringError();
}

Panic::operator PanickingError() {
    return PanickingError();
}

Log::operator LoggingError() {
    return LoggingError();
}

//Ignore error::ignore;

// void ErrorReporter::report(Error const &e) {
//     if (this->has_error) {
//         return;
//     }

//     this->has_error = true;
//     this->handler(e);
// }

void BasicError::fmt(io::Writer &out, error err) const {
    out.write(msg, err);
}

// static void panic_handler(const Error &e) {

// }

//constinit ErrorReporter error::panic = ErrorReporter(&panic_handler);

//static void ignore_func(Error &err) {}

//operator error();

// error::Ignore error::ignore;

// error2::Panic error2::panic;
// error2::Log error2::log;

// const Error lib::ErrUnspecified("unspecified error");

// void error2::Panic::operator() (Error const &err) {
//     if (&err == &io::EOF) {
//         this->ref = &err;
//         return;
//     }
//     lib::panic(err.msg);
// }

// void error2::Panic::operator() (str msg) {
//     lib::panic(msg);
// }

// void error2::Log::operator() (Error const &err) {
//     fmt::fprintf(stderr, "error: %s\n", err.msg);
// }

// void error2::Log::operator() (str msg) {
//     fmt::fprintf(stderr, "error: %s\n", msg);
// }

// void error2::IgnoreOneImpl::operator() (Error const &err) {
//     if (&err == this->ignore) {
//         parent.ref = &err;
//         return;
//     }
//     parent(err);
// }

// void error2::IgnoreOneImpl::operator() (str msg) {
//     parent(msg);
// }

// error2::IgnoreAll error2::ignore() {
//     return IgnoreAll();
// }

// error2::IgnoreOne error2::ignore(Error const& err) {
//     return IgnoreOne(*this, &err);
// }


// void error2::fmt(fmt::Fmt& out) const {
//     if (!ref) {
//         out << "ok";
//     } else {
//         out << ref->msg;
//     }
// }

// bool error2::handle(Error const& err) {
//     if (ref == &err) {
//         ref = nil;
//         return true;
//     }

//     return false;
// }

// void BasicError::describe(io::OStream &out) const {
//     out.write(msg, error2::ignore());
// }

// ErrorRecorder::ErrorRecorder() : error(error_reporter) {}

// void ErrorRecorder::handle_error(const Error &e) {
//     this->msg = fmt::stringify(e);
// }

void ErrorRecorder::report(Error &e) {
    this->report((Error const&) e);
}

void ErrorRecorder::fmt(io::Writer &out, error err) const {
    if (!this->has_error) {
        out.write("ok", err);
        return;
    }

    out.write(this->msg, err);
}

bool ErrorRecorder::is(Error const &other) {
  if (!this->has_error) {
    return false;
  }
  if (this->type != other.type) {
    return false;
  }
  if (this->msg != fmt::stringify(other)) {
    return false;
  }
  return true;
}

bool ErrorRecorder::operator==(ErrorRecorder const &other) const {
    if (!has_error) {
        return !other.has_error;
    }

  return other.has_error && type == other.type && msg == other.msg;
}


SavedError::SavedError(TypeID id, str msg) : Error(id), msg(msg) {}

void SavedError::fmt(io::Writer &out, error err) const {
    out.write(msg, err);
}


SavedError ErrorRecorder::to_error() const {
  return SavedError(type, msg);
}

void ErrorRecorder::report(Error const &e) {
    this->has_error = true;
    this->type = e.type;
    this->msg = fmt::stringify(e);
}
