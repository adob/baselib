# Baselib's CMakeLists.txt loads the buildtool dependency before this file.
# Register module sources without invoking baselib's conventional source build.
include_guard(GLOBAL)

# TODO: Find and link Threads only when the requested module closure uses lib.async.
find_package(Threads REQUIRED)
# TODO: Find and link fmt only when the requested module closure uses lib.fmt.
find_package(fmt 12 CONFIG REQUIRED)

buildtool_register_project(baselib SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
add_library(baselib::baselib ALIAS baselib)
# Baselib's public module API requires C++23; CMake propagates this minimum to users.
target_compile_features(baselib PUBLIC cxx_std_23)
# GNU language mode defines the legacy `unix` macro, colliding with time::unix.
set_target_properties(baselib PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(baselib PUBLIC Threads::Threads fmt::fmt-header-only)
target_include_directories(baselib PRIVATE
  "$<$<COMPILE_LANGUAGE:C>:${CMAKE_CURRENT_LIST_DIR}/../deps/libdebugme/include>")

# Keep the test runner available when the module configuration bypasses the
# conventional source build. Build its implementation dependencies as modules.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_library(BASELIB_ELF_LIBRARY NAMES elf)
  if(BASELIB_ELF_LIBRARY)
    # The test runner imports lib.debug, whose stack traces use cpptrace.
    find_package(cpptrace CONFIG REQUIRED)
    buildtool_add_library(baselib_test_main
      LIBRARY baselib::baselib
      SOURCES "${CMAKE_CURRENT_LIST_DIR}/../lib/testing/testmain.cc")
    add_library(baselib::test_main ALIAS baselib_test_main)
    target_link_libraries(baselib_test_main INTERFACE
      "${BASELIB_ELF_LIBRARY}" ${CMAKE_DL_LIBS} cpptrace::cpptrace)
  else()
    message(STATUS "libelf not found; skipping baselib::test_main")
  endif()
endif()
