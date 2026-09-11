#pragma once

import lib.print;
import "os/stdio.h"; // The macros below access these streams in the caller.

// Named modules cannot export these expression-style macros.
#define print  (::prettyprint::Print(::lib::os::stdout))*
#define eprint (::prettyprint::Print(::lib::os::stderr))*
