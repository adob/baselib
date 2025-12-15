cmake_minimum_required(VERSION 3.16)
project(baselib VERSION 0.1.0 LANGUAGES CXX)

set(BASELIB_ROOT ${CMAKE_CURRENT_LIST_DIR})

file(GLOB BASELIB_SOURCES 
    ${BASELIB_ROOT}/lib/*.cc
    ${BASELIB_ROOT}/lib/errors/*.cc
    ${BASELIB_ROOT}/lib/fmt/*.cc
    ${BASELIB_ROOT}/lib/io/*.cc
    ${BASELIB_ROOT}/lib/os/*.cc
    ${BASELIB_ROOT}/lib/math/*.cc
    ${BASELIB_ROOT}/lib/strconv/*.cc
    ${BASELIB_ROOT}/lib/strings/*.cc
    ${BASELIB_ROOT}/lib/sync/*.cc
    ${BASELIB_ROOT}/lib/time/*.cc
    ${BASELIB_ROOT}/lib/unicode/*.cc
    ${BASELIB_ROOT}/lib/utf8/*.cc
    ${BASELIB_ROOT}/lib/varint/*.cc

    ${BASELIB_ROOT}/deps/fmt/src/format.cc
)

foreach(src_file IN LISTS BASELIB_SOURCES)
  if(src_file MATCHES ".*_(test|experimental|benchmark|arduino)\\.cc$")
    list(REMOVE_ITEM BASELIB_SOURCES ${src_file})
  endif()
endforeach()

add_library(baselib STATIC
    ${BASELIB_SOURCES}
)

add_library(baselib::baselib ALIAS baselib)


target_include_directories(baselib
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/config/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/circular_buffer/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/static_assert/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/type_traits/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/throw_exception/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/assert/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/core/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/move/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/concept_check/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/preprocessor/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/fmt/include/)


#target_include_directories(baselib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../deps/eigen)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/math/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/variant/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/type_index/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/container_hash/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/describe/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/mp11/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/mpl/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/detail/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/utility/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/integer/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/serialization/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/fusion/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/function_types/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/io/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/smart_ptr/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/asio/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/align/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/iostreams/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/range/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/iterator/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/foreach/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/bind/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/optional/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/variant2/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/spirit/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/predef/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/array/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/function/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/filesystem/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/system/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/format/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/property_tree/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/any/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/multi_index/include/)
target_include_directories(baselib PRIVATE ${BASELIB_ROOT}/deps/boost/libs/tuple/include/)


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_26)


if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wconversion
            -Wno-sign-compare -Wno-deprecated -Wno-sign-conversion
            -Wno-missing-field-initializers
            -Werror=shift-count-overflow
            -Werror=return-type)
endif()

# Optional: warnings / defs / options propagated to dependents
# target_compile_definitions(baselib PUBLIC BASELIB_USE_SOMETHING=1)

# ---- Install + export (nice to have even if you mostly use add_subdirectory) ----
include(GNUInstallDirs)

install(TARGETS baselib
  EXPORT baselibTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES baselib.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


install(EXPORT baselibTargets
  NAMESPACE baselib::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/baselib
)

# # Optional package config (only needed if someone will find_package(baselib))
# include(CMakePackageConfigHelpers)
# write_basic_package_version_file(
#   "${CMAKE_CURRENT_BINARY_DIR}/baselibConfigVersion.cmake"
#   VERSION ${PROJECT_VERSION}
#   COMPATIBILITY SameMajorVersion
# )

# configure_package_config_file(
#   "${CMAKE_CURRENT_LIST_DIR}/cmake/baselibConfig.cmake.in"
#   "${CMAKE_CURRENT_BINARY_DIR}/baselibConfig.cmake"
#   INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/baselib
# )

# install(FILES
#   "${CMAKE_CURRENT_BINARY_DIR}/baselibConfig.cmake"
#   "${CMAKE_CURRENT_BINARY_DIR}/baselibConfigVersion.cmake"
#   DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/baselib
# )
