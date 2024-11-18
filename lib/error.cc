#include "error.h"
//#include "lib/fmt/fmt.h"
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


void ErrorReporter::report(Error const &e) {
    this->has_error = true;
    this->handler(e);
}

void BasicError::describe(io::OStream &out) const {
    out.write(msg, error::ignore);
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