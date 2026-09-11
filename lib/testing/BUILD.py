LDFLAGS = ["-lelf"]

# Separate runner entry points and the CMake-only replacement for debug.cc.
EXPLICIT_SOURCES = ["testmain.cc", "benchmain.cc", "debug_stub.cc"]
