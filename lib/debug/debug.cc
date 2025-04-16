// #define BACKWARD_HAS_UNWIND 1

// #define BACKWARD_HAS_DW 1
// #define BACKWARD_HAS_BFD 0
// #define BACKWARD_HAS_DWARF 0
// #define BACKWARD_HAS_BACKTRACE 0
// #define BACKWARD_HAS_BACKTRACE_SYMBOL 0
// #define BACKWARD_HAS_LIBUNWIND 0
// #include "../../deps/backward-cpp/backward.hpp"

#include "lib/fmt/fmt.h"
#include "lib/io/io_stream.h"
#include "lib/sync/lock.h"
#include "lib/sync/mutex.h"
#include <cpptrace/basic.hpp>
#include <cpptrace/forward.hpp>
#include <execinfo.h>
#include <exception>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <typeinfo>
#include <cxxabi.h>
#include <cpptrace/cpptrace.hpp>
#include <cpptrace/formatting.hpp>

// namespace cpptrace::detail {
//     std::vector<stacktrace_frame> resolve_frames(const std::vector<object_frame>& frames);
//     std::vector<stacktrace_frame> resolve_frames(const std::vector<frame_ptr>& frames);
// }


extern "C" {
    #include "../../deps/libdebugme/include/debugme.h"
}

#include "./debug.h"

#include "lib/types.h"
#include "lib/panic.h"
#include "lib/exceptions.h"
#include "lib/fmt.h"
#include "lib/mem.h"
//#include "../print.h"

using namespace lib;
using namespace debug;

extern "C" {
    char*
    __cxa_demangle(const char* __mangled_name, char* __output_buffer,
                   size_t* __length, int* __status);

}

static String stringify_stack_trace() {
    //fmt::printf("capture stack trace start\n");
    cpptrace::stacktrace st = cpptrace::generate_trace();
    // st.print_with_snippets(std::cerr);

    return cpptrace::formatter().snippets(true).format(st, true);

    // return st.to_string(true);
    
    //fmt::printf("capture stack trace end\n");
}

void debug::print_exception(std::exception_ptr excep) {
    fmt::fprintf(os::stderr, format_exception(excep));
}

String debug::format_exception(std::exception_ptr excep) {
    io::Buffer b;
    try {
        std::rethrow_exception(excep);
    } catch (Error const& err) {
        fmt::fprintf(b, "error: %s", err);
    }  catch (Exception &e) {
        // std::string type_name;

        fmt::fprintf(b, "%s\n", e);
        
        b.write(stringify_stack_trace());
        
        //return;
    } catch (std::exception const& ex) {
        fmt::fprintf(stderr, "exception: %s", ex.what());

        b.write(stringify_stack_trace());

    } catch (...) {
        std::type_info *t = abi::__cxa_current_exception_type();
        //std::string type = demangle(t->name());
        //std::string type = backward::demangler.demangle(t->name());
        fmt::fprintf(stderr, "exception of type %s.\n", t);
        
        // {backward::Printer p;
        // backward::StackTrace st;
        // st.load_here(1024);
        // }p.print(st, stderr);
        b.write(stringify_stack_trace());
    }

    return b.to_string();
}

struct sigaction old_fpe, old_segv, old_pipe, old_quit;
//bool is_crashing = false;

sync::Mutex crash_mu;

static void crash_handler() {
    // prevent two threads that crash around the same time from printing out at the same time
    sync::Lock lock(crash_mu);

    // if (is_crashing) {
    //     fprintf(stderr, "\n\nCrashed while crashing!!!\n");
    //     std::abort();
    // }
    // is_crashing = true;
    // sigaction(SIGFPE, &old_fpe, nil);
    // sigaction(SIGSEGV, &old_segv, nil);
    // sigaction(SIGPIPE, &old_pipe, nil);
    // sigaction(SIGQUIT, &old_quit, nil);
    // std::set_terminate(nil);

    ::close(1);

    String s = debug::format_exception(std::current_exception());    
    fmt::fprintf(os::stderr, "\nTerminated due to %s\n", s);

    std::abort();
}

static void sigfpe_handler(int /*signum*/, siginfo_t */*siginfo*/, void*) {
    fprintf(stderr, "caught SIGFPE\n");
    
    sigset_t x;
    sigemptyset (&x);
    sigaddset(&x, SIGFPE);
    sigprocmask(SIG_UNBLOCK, &x, NULL);
    
    panic("SIGFPE");
}

static void sigpipe_handler(int /*signum*/, siginfo_t */*siginfo*/, void*) {
    fprintf(stderr, "caught SIGPIPE\n");
    
    sigset_t x;
    sigemptyset (&x);
    sigaddset(&x, SIGPIPE);
    sigprocmask(SIG_UNBLOCK, &x, NULL);
    
    panic("SIGPIPE");
}

static void sigsegv_handler(int /*signum*/, siginfo_t *siginfo, void*) {
    sigset_t x;
    sigemptyset (&x);
    sigaddset(&x, SIGSEGV);
    sigprocmask(SIG_UNBLOCK, &x, NULL);

    if (siginfo->si_addr == 0) {
        throw exceptions::NullMemAccess();
    } else {
        throw exceptions::BadMemAccess(siginfo->si_addr);
    }
    (void) siginfo;
}

static void sigquit_handler(int /*signum*/, siginfo_t */*siginfo*/, void*) {
    fmt::fprintf(os::stderr, "caught SIGQUIT\n");
    debugme_debug(0, "");
}



void debug::init() {
    // https://github.com/yugr/libdebugme

    static bool inited = false;
    if (inited) {
        return;
    }
    inited = true;
    
    std::set_terminate(crash_handler);
    struct sigaction action = {};
    action.sa_sigaction = sigfpe_handler;
    action.sa_flags = SA_SIGINFO;

    int err = sigaction(SIGFPE, &action, &old_fpe);
    if (err) {
        perror("debug::init: error installing SIGFPE handler");
        exit(1);
    }
    
    action.sa_sigaction = sigsegv_handler;    
    err = sigaction(SIGSEGV, &action, &old_segv);
    if (err) {
        perror("debug::init: error installing SIGFPE handler");
        exit(1);
    }
    
    action.sa_sigaction = sigpipe_handler;
    err = sigaction(SIGPIPE, &action, &old_pipe);
    if (err) {
        perror("debug::init: error installing SIGPIPE handler");
        exit(1);
    }

    action.sa_sigaction = sigquit_handler;
    err = sigaction(SIGQUIT, &action, &old_quit);
    if (err) {
        perror("debug::init: error installing SIGQUIT handler");
        exit(1);
    }

    stack_t stack = {
        .ss_sp    = mem::alloc(SIGSTKSZ),
        .ss_flags = 0,
        .ss_size = size_t(SIGSTKSZ),
    };
    err = sigaltstack(&stack, nil);
    if (err) {
         perror("debug::init: sigaltstack");
    }
}

Backtrace debug::backtrace(int /*offset*/) {
    panic("unimplemented");
    return {};
}

void Backtrace::fmt(io::Writer &/*out*/, error) const {
    // backward::Printer p;
        
        
    // //p.print(this->addrs, stderr);
    // out.write("Backtrace::fmt unimplemented", err);
}

SymInfo debug::get_symbol_info(void *ptr) {
    std::vector<cpptrace::frame_ptr> raw_frames = { cpptrace::frame_ptr(ptr)};
    cpptrace::raw_trace trace;
    trace.frames = raw_frames;
    
    cpptrace::stacktrace st = trace.resolve();
    if (st.frames.size() == 0) {
        panic("resolved stracktrace has no frames");
    }

    cpptrace::stacktrace_frame resolved = st.frames[0];

    return SymInfo {
        .demangled = resolved.symbol,
        .filename  = resolved.filename,
        .lineno    = int(resolved.line.value_or(-1)),
        .colno     = int(resolved.column.value_or(-1)),
    } ;


    //auto result = cpptrace::detail::resolve_frames(frames);

    //fmt::printf("result: %v\n", st.frames[0].to_string());

    //panic("unimplemented");
    //return {};
    // backward::Trace trace(ptr, 0);

    // backward::TraceResolver resolver;
    // backward::ResolvedTrace resolved = resolver.resolve(trace);

    // //fmt::printf("function %q\n", resolved.object_function);
    // // fmt::printf("filename %q\n", resolved.source.filename);
    
    // return SymInfo {
    //     .demangled = resolved.object_function,
    //     .filename  = resolved.source.filename,
    //     .lineno    = int(resolved.source.line),
    //     .colno     = int(resolved.source.col),
    // } ;
}