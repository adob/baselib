#pragma once

import lib.print;
import lib.os.stdio;

// Named modules cannot export these expression-style macros.
#define print  (::prettyprint::Print(::lib::os::stdout))*
#define eprint (::prettyprint::Print(::lib::os::stderr))*
