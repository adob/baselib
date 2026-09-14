# Top-level C++ modules

Import top-level APIs by name, for example:

```cpp
import lib.array;
import lib.error;
import lib.inline_string;
```

`import lib;` provides the former umbrella header's APIs. The modules
re-export their public module dependencies. Declarations use `extern "C++"`
to remain compatible with forward declarations and definitions in the
remaining header-based code.

Headers under subdirectories remain headers. The I/O, formatting, errors, and
standard-stream headers are imported as header units where used by `lib`
and `lib.print`. Consumers use those same header units to avoid mixing imported
definitions with textual definitions in GCC.

Two textual public headers remain intentionally:

- `assert.h` defines `LIB_CHECK`, which is evaluated when each file is compiled.
- `print.h` imports `lib.print` and defines the `print` and `eprint` expression
  macros. Named modules cannot export macros.

The `*_impl.cc` files hold out-of-line implementations that depend on other
library APIs. Their small `*_impl.h` companion headers tell buildtool to discover
and link those source files; they are build dependencies rather than public APIs.
Buildtool likewise discovers companion source files when a project header is
imported as a header unit.

`lib/modules_test.cc` exercises the combined public imports. This migration has
been validated with the workspace GCC toolchain; Clang has not been validated.
