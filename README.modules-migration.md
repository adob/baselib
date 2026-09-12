# Recursive module migration

This migration converts the C++ APIs under `lib/` to named modules. Vendored C
code remains C. Tests and executable entry points remain ordinary translation
units that import the library.

## Layout

A former `lib/foo/foo.h` normally becomes module `lib.foo` in `lib/foo/foo.cc`.
Other interfaces use their path, for example `lib.sync.mutex` and
`lib.unicode.tables`. Consumers use named imports. Each module exports its own API; dependencies
on other `lib.*` modules are ordinary imports. Consumers must directly import
each module whose declarations they use. Same-module interface partitions
remain re-exported, as do the separately documented header-unit workarounds.

Where an existing `.cc` contains out-of-line definitions, those definitions move
to `*_impl.cc`. A small `*_impl.h` marker remains for buildtool's companion-source
link discovery. The marker contains no public API. These implementation files
import their interface and retain the existing C++ linkage. Interfaces use
`export extern "C++"` to remain compatible during the transition.

`assert.h` and `print.h` retain macros: named module imports cannot export macros.
Platform-specific code retains its preprocessor conditions and build tags.

## Compatibility and inactive sources

Public template dependencies such as `<future>`, `<deque>`, and Boost's circular
buffer are re-exported where Clang needs their definitions in an importing TU.
These workarounds are grouped under TODO comments; GCC 15 also needs `<typeinfo>`
visible when instantiating formatting templates. `Writer::direct_read` remains
inline because GCC 15 otherwise omits its RTTI after removing module re-exports.
Namespace-scope API constants use `inline constexpr` so GCC can export them from
the C++ linkage block. The error constraint uses a named concept around the
compiler trait to avoid Clang's imported trait-initializer issue and GCC's ban on
putting the raw trait directly in a function signature.

`lib.strings.two_way` replaces `strings/2way.h`, since a module component cannot
start with a digit. POSIX file-mode declarations are in `lib.os.file`.
`lib.serial` contains the serial port interface; its implementation keeps the
`+linux` tag. Zephyr USB and experimental channel interfaces retain their tags;
buildtool now resolves active tagged module filenames as a fallback, rejecting
ambiguous matches. Unqualified module layouts keep their existing priority.

The unused, incomplete `fmt.inlines.h` and `strconv/ftoaryu.h` drafts are preserved
under `archive/` as text. The latter contains untranslated Go syntax. Neither was
part of the active library. Generated compatibility headers, macro headers, and
vendored C code are not converted into named modules.

## Validation

- Full recursive library rebuilds pass with GCC and the patched Clang wrapper.
- All recursive test sources compile with both compilers.
- Core, errors, I/O, strings, testing, and UTF-8 suites pass. New async and map
  consumer tests pass with both compilers; a modules-only smoke program also runs
  successfully with both compilers.
- The full test run is not green: the formatting suite has the same 43 failing
  `sprintf` cases as the committed baseline, GCC's floating-point conversion test
  fails in both versions, and Clang reaches the existing `panic("unimplemented")`
  in error unwrapping. `test_once` times out in both the baseline and migrated
  library: its temporary `go` object joins before the receiver starts.
- Buildtool's test suite passes: 199 passed, 18 skipped, including tests for
  active tags, ambiguous variants, and unqualified module priority.

Logs are `/tmp/modules-full-{clang,gcc}.log`,
`/tmp/modules-tests-clang-final.log`, `/tmp/modules-tests-gcc.log`,
`/tmp/modules-tests-{clang,gcc}-tail.log`, and
`/tmp/modules-template-tests-{clang,gcc}.log`. Baseline comparisons live under
`/tmp/baselib-module-baseline` with logs `/tmp/modules-baseline-*.log`.

Cross-target branches require their SDKs and are not validated by a Linux build.
The legacy CMake/GN builds are not validated by this buildtool migration.

## Module interfaces

| Module | Interface |
| --- | --- |
| `lib.array` | [lib/array.cc](lib/array.cc) |
| `lib.async` | [lib/async/async.cc](lib/async/async.cc) |
| `lib.base` | [lib/base.cc](lib/base.cc) |
| `lib.bitflag` | [lib/bitflag.cc](lib/bitflag.cc) |
| `lib.buf` | [lib/buf.cc](lib/buf.cc) |
| `lib.concepts` | [lib/concepts.cc](lib/concepts.cc) |
| `lib.debug` | [lib/debug/debug.cc](lib/debug/debug.cc) |
| `lib.error` | [lib/error.cc](lib/error.cc) |
| `lib.errors` | [lib/errors/errors.cc](lib/errors/errors.cc) |
| `lib.errors.join` | [lib/errors/join.cc](lib/errors/join.cc) |
| `lib.exception` | [lib/exception.cc](lib/exception.cc) |
| `lib.exceptions` | [lib/exceptions.cc](lib/exceptions.cc) |
| `lib.fallback` | [lib/fallback.cc](lib/fallback.cc) |
| `lib.filepath.path` | [lib/filepath/path.cc](lib/filepath/path.cc) |
| `lib.flag` | [lib/flag/flag.cc](lib/flag/flag.cc) |
| `lib.fmt` | [lib/fmt/fmt.cc](lib/fmt/fmt.cc) |
| `lib.fs` | [lib/fs/fs.cc](lib/fs/fs.cc) |
| `lib.inline_string` | [lib/inline_string.cc](lib/inline_string.cc) |
| `lib.io` | [lib/io/io.cc](lib/io/io.cc) |
| `lib.io.pipe` | [lib/io/pipe.cc](lib/io/pipe.cc) |
| `lib.io.util` | [lib/io/util.cc](lib/io/util.cc) |
| `lib.math:bits` | [lib/math/bits+experimental.cc](lib/math/bits+experimental.cc) |
| `lib.math.bits` | [lib/math/bits.cc](lib/math/bits.cc) |
| `lib.math` | [lib/math/math+experimental.cc](lib/math/math+experimental.cc) |
| `lib.math` | [lib/math/math.cc](lib/math/math.cc) |
| `lib.math:matrix` | [lib/math/matrix+experimental.cc](lib/math/matrix+experimental.cc) |
| `lib.math.matrix` | [lib/math/matrix.cc](lib/math/matrix.cc) |
| `lib.mem` | [lib/mem.cc](lib/mem.cc) |
| `lib.os.error` | [lib/os/error.cc](lib/os/error.cc) |
| `lib.os.file` | [lib/os/file.cc](lib/os/file.cc) |
| `lib.os.pipe` | [lib/os/pipe.cc](lib/os/pipe.cc) |
| `lib.os.stat` | [lib/os/stat.cc](lib/os/stat.cc) |
| `lib.os.stdio` | [lib/os/stdio.cc](lib/os/stdio.cc) |
| `lib.os.types` | [lib/os/types.cc](lib/os/types.cc) |
| `lib.panic` | [lib/panic.cc](lib/panic.cc) |
| `lib.print` | [lib/print.cc](lib/print.cc) |
| `lib.runtime.debug` | [lib/runtime/debug.cc](lib/runtime/debug.cc) |
| `lib.runtime.mstats` | [lib/runtime/mstats.cc](lib/runtime/mstats.cc) |
| `lib.runtime.symtab` | [lib/runtime/symtab.cc](lib/runtime/symtab.cc) |
| `lib.serial` | [lib/serial/serial.cc](lib/serial/serial.cc) |
| `lib.serial.serial_listener` | [lib/serial/serial_listener.cc](lib/serial/serial_listener.cc) |
| `lib.serial.usbio` | [lib/serial/usbio+zephyr.cc](lib/serial/usbio+zephyr.cc) |
| `lib.str` | [lib/str.cc](lib/str.cc) |
| `lib.strconv.atoi` | [lib/strconv/atoi.cc](lib/strconv/atoi.cc) |
| `lib.strconv.ftoa` | [lib/strconv/ftoa.cc](lib/strconv/ftoa.cc) |
| `lib.strconv.isprint` | [lib/strconv/isprint.cc](lib/strconv/isprint.cc) |
| `lib.strconv.itoa` | [lib/strconv/itoa.cc](lib/strconv/itoa.cc) |
| `lib.strconv.quote` | [lib/strconv/quote.cc](lib/strconv/quote.cc) |
| `lib.strings` | [lib/strings/strings.cc](lib/strings/strings.cc) |
| `lib.strings.two_way` | [lib/strings/two_way.cc](lib/strings/two_way.cc) |
| `lib.sync.atomic` | [lib/sync/atomic.cc](lib/sync/atomic.cc) |
| `lib.sync.chan` | [lib/sync/chan.cc](lib/sync/chan.cc) |
| `lib.sync.chan_lockreduced` | [lib/sync/chan_lockreduced+experimental.cc](lib/sync/chan_lockreduced+experimental.cc) |
| `lib.sync.cond` | [lib/sync/cond.cc](lib/sync/cond.cc) |
| `lib.sync.gang` | [lib/sync/gang.cc](lib/sync/gang.cc) |
| `lib.sync.go` | [lib/sync/go.cc](lib/sync/go.cc) |
| `lib.sync.lock` | [lib/sync/lock.cc](lib/sync/lock.cc) |
| `lib.sync.map` | [lib/sync/map.cc](lib/sync/map.cc) |
| `lib.sync.mutex` | [lib/sync/mutex.cc](lib/sync/mutex.cc) |
| `lib.sync.once` | [lib/sync/once.cc](lib/sync/once.cc) |
| `lib.sync.rwmutex` | [lib/sync/rwmutex.cc](lib/sync/rwmutex.cc) |
| `lib.sync.waitgroup` | [lib/sync/waitgroup.cc](lib/sync/waitgroup.cc) |
| `lib.testing.benchmark` | [lib/testing/benchmark.cc](lib/testing/benchmark.cc) |
| `lib.testing.example` | [lib/testing/example.cc](lib/testing/example.cc) |
| `lib.testing.func` | [lib/testing/func.cc](lib/testing/func.cc) |
| `lib.testing.fuzz` | [lib/testing/fuzz.cc](lib/testing/fuzz.cc) |
| `lib.testing.matcher` | [lib/testing/matcher.cc](lib/testing/matcher.cc) |
| `lib.testing` | [lib/testing/testing.cc](lib/testing/testing.cc) |
| `lib.time` | [lib/time/time.cc](lib/time/time.cc) |
| `lib.type_id` | [lib/type_id.cc](lib/type_id.cc) |
| `lib.types` | [lib/types.cc](lib/types.cc) |
| `lib.unicode.graphic` | [lib/unicode/graphic.cc](lib/unicode/graphic.cc) |
| `lib.unicode.letter` | [lib/unicode/letter.cc](lib/unicode/letter.cc) |
| `lib.unicode.tables` | [lib/unicode/tables.cc](lib/unicode/tables.cc) |
| `lib.unicode` | [lib/unicode/unicode.cc](lib/unicode/unicode.cc) |
| `lib.utf8.codes` | [lib/utf8/codes.cc](lib/utf8/codes.cc) |
| `lib.utf8.decode` | [lib/utf8/decode.cc](lib/utf8/decode.cc) |
| `lib.utf8.encode` | [lib/utf8/encode.cc](lib/utf8/encode.cc) |
| `lib.utf8` | [lib/utf8/utf8.cc](lib/utf8/utf8.cc) |
| `lib.utils` | [lib/utils.cc](lib/utils.cc) |
| `lib.varint` | [lib/varint/varint.cc](lib/varint/varint.cc) |
