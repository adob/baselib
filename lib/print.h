#pragma once

import lib.print;
import lib.io;

// Named modules cannot export these expression-style macros.
#define print  (::prettyprint::Print(::lib::io::out))*
#define eprint (::prettyprint::Print(::lib::io::err))*
