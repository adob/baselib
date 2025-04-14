#pragma once

#include "lib/str.h"
#include "lib/types.h"

namespace lib::runtime {
    struct Frame {
        // PC is the program counter for the location in this frame.
        // For a frame that calls another frame, this will be the
        // program counter of a call instruction. Because of inlining,
        // multiple frames may have the same PC value, but different
        // symbolic information.
        uintptr pc = 0;

        // // Func is the Func value of this call frame. This may be nil
        // // for non-Go code or fully inlined functions.
        // Func *Func

        // Function is the package path-qualified function name of
        // this call frame. If non-empty, this string uniquely
        // identifies a single function in the program.
        // This may be the empty string if not known.
        // If Func is not nil then Function == Func.Name().
        String function;

        // File and Line are the file name and line number of the
        // location in this frame. For non-leaf frames, this will be
        // the location of a call. These may be the empty string and
        // zero, respectively, if not known. The file name uses
        // forward slashes, even on Windows.
        String file;
        int line = 0;

        // // startLine is the line number of the beginning of the function in
        // // this frame. Specifically, it is the line number of the func keyword
        // // for Go functions. Note that //line directives can change the
        // // filename and/or line number arbitrarily within a function, meaning
        // // that the Line - startLine offset is not always meaningful.
        // //
        // // This may be zero if not known.
        // startLine int

        // // Entry point program counter for the function; may be zero
        // // if not known. If Func is not nil then Entry ==
        // // Func.Entry().
        // Entry uintptr

        // // The runtime's internal view of the function. This field
        // // is set (funcInfo.valid() returns true) only for Go functions,
        // // not for C functions.
        // funcInfo funcInfo
    } ;
}