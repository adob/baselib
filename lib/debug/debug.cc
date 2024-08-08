#define BACKWARD_HAS_UNWIND 1
#define BACKWARD_HAS_DW 1

#define BACKWARD_HAS_BFD 0
#define BACKWARD_HAS_DWARF 0
#define BACKWARD_HAS_BACKTRACE 0
#define BACKWARD_HAS_BACKTRACE_SYMBOL 0
#define BACKWARD_HAS_LIBUNWIND 0
#include "../../deps/backward-cpp/backward.hpp"

#include <execinfo.h>
#include <exception>
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <typeinfo>


extern "C" {
    #include "../../deps/libdebugme/include/debugme.h"
}

#include "./debug.h"

#include "lib/types.h"
#include "lib/panic.h"
#include "lib/exceptions.h"
#include "lib/fmt.h"
#include "lib/mem.h"
//#include "sharedlib/print.h"

using namespace lib;

static void crash_handler() {
    
    fmt::fprintf(stderr, "Terminated due to unhandled");
    
    std::exception_ptr excep = std::current_exception();
    try {
        std::rethrow_exception(excep);
    } catch (error const& err) {
        fmt::fprintf(stderr, " error: %s", err);
    }  catch (Exception &e) {
        //fmt::printf("ptr %d\n", (uintptr_t) e.msg.buffer.data);
        fmt::fprintf(stderr, " exception %q\n\n", e.msg);
        
        backward::Printer p;
        if (e.stacktrace) {
            p.print(*e.stacktrace, stderr);
        } else {
            backward::StackTrace st;
            st.load_here(1024);
            p.print(st, stderr);
        }
        
        //return;
    } catch (std::exception const& ex) {
        fmt::fprintf(stderr, " exception: %s", ex.what());

        backward::Printer p;
        backward::StackTrace st;
        st.load_here(1024);
        p.print(st, stderr);
    } catch (...) {
        std::type_info *t = abi::__cxa_current_exception_type();
        //std::string type = demangle(t->name());
        //std::string type = backward::demangler.demangle(t->name());
        fmt::fprintf(stderr, " exception of type %s.\n", t);
        
        backward::Printer p;
        backward::StackTrace st;
        st.load_here(1024);
        p.print(st, stderr);
    }

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

static void sigquit_handler(int /*signum*/, siginfo_t *siginfo, void*) {
    fmt::fprintf(io::err, "caught SIGQUIT\n");
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

    int err = sigaction(SIGFPE, &action, nil);
    if (err) {
        perror("debug::init: error installing SIGFPE handler");
        exit(1);
    }
    
    action.sa_sigaction = sigsegv_handler;    
    err = sigaction(SIGSEGV, &action, nil);
    if (err) {
        perror("debug::init: error installing SIGFPE handler");
        exit(1);
    }
    
    action.sa_sigaction = sigpipe_handler;
    err = sigaction(SIGPIPE, &action, nil);
    if (err) {
        perror("debug::init: error installing SIGPIPE handler");
        exit(1);
    }

    action.sa_sigaction = sigquit_handler;
    err = sigaction(SIGQUIT, &action, nil);
    if (err) {
        perror("debug::init: error installing SIGQUIT handler");
        exit(1);
    }

    stack_t stack = {
        .ss_sp    = mem::alloc(SIGSTKSZ),
        .ss_flags = 0,
        .ss_size = SIGSTKSZ,
    };
    err = sigaltstack(&stack, nil);
    if (err) {
         perror("debug::init: sigaltstack");
    }
}
