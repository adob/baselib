#pragma once

#include <functional>

#include "lib/str.h"

namespace lib::flag {
    struct FlagSet {
        // void var(str name, int value, str usage) {

        // }
    } ;

    // parsed reports whether f.Parse has been called.
    bool parsed();

    // parse parses the command-line flags from [os.Args][1:]. Must be called
    // after all flags are defined and before flags are accessed by the program.
    void parse();

    // Usage prints a usage message documenting all defined command-line flags
    // to [CommandLine]'s output, which by default is [os.Stderr].
    // It is called when an error occurs while parsing flags.
    // The function is a variable that may be changed to point to a custom function.
    // By default it prints a simple header and calls [PrintDefaults]; for details about the
    // format of the output and how to control it, see the documentation for [PrintDefaults].
    // Custom usage functions may choose to exit the program; by default exiting
    // happens anyway as the command line's error handling strategy is set to
    // [ExitOnError].
    extern std::function<void()> usage;

    template <typename T>
    T *define(str name, T value, str usage) {
        return new T(value);
    }
}