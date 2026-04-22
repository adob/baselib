cmake_minimum_required(VERSION 3.16)

get_filename_component(
  CURRENT_DIRNAME
  ${CMAKE_CURRENT_LIST_DIR}
  NAME
)

project(${CURRENT_DIRNAME} VERSION 0.0.0 LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/CPM.cmake)

set(BASELIB_BOOST_VERSION "1.89.0" CACHE STRING "Boost version used by baselib CPM dependencies")

file(GLOB SOURCES 
    ${CMAKE_CURRENT_LIST_DIR}/lib/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/errors/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/filepath/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/fmt/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/fs/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/io/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/os/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/math/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/serial/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/strconv/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/strings/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/sync/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/time/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/unicode/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/utf8/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/varint/*.cc

    ${CMAKE_CURRENT_LIST_DIR}/deps/fmt/src/format.cc
)

foreach(src_file IN LISTS SOURCES)
  if(src_file MATCHES ".*_(test|experimental|benchmark|arduino)\\.cc$")
    list(REMOVE_ITEM SOURCES ${src_file})
  endif()

  if(src_file MATCHES "(^|[^A-Za-z0-9_])example([^A-Za-z0-9_]|$)")
    list(REMOVE_ITEM SOURCES ${src_file})
  endif()
endforeach()

add_library(${PROJECT_NAME} STATIC
    ${SOURCES}
)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

#set_target_properties(baselib PROPERTIES
  #CXX_STANDARD 26
  #CXX_STANDARD_REQUIRED YES
  #CXX_EXTENSIONS NO
#)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wconversion
            -Wno-sign-compare -Wno-deprecated -Wno-sign-conversion
            -Wno-missing-field-initializers
            -Werror=shift-count-overflow
            -Werror=return-type)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
  $<INSTALL_INTERFACE:include>
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/fmt/include/)

# set(BASELIB_BOOST_LIBRARIES
#   config
#   circular_buffer
#   static_assert
#   type_traits
#   throw_exception
#   assert
#   core
#   move
#   concept_check
#   preprocessor
#   math
#   variant
#   type_index
#   container_hash
#   describe
#   mp11
#   mpl
#   detail
#   utility
#   integer
#   serialization
#   fusion
#   function_types
#   io
#   smart_ptr
#   asio
#   align
#   iostreams
#   range
#   iterator
#   foreach
#   bind
#   optional
#   variant2
#   spirit
#   predef
#   array
#   function
#   filesystem
#   system
#   format
#   property_tree
#   any
#   multi_index
#   tuple
#   atomic
# )

# function(baselib_add_boost_library boost_lib)
#   CPMAddPackage(
#     NAME boost_${boost_lib}
#     GITHUB_REPOSITORY boostorg/${boost_lib}
#     GIT_TAG boost-${BASELIB_BOOST_VERSION}
#     DOWNLOAD_ONLY YES
#   )

#   set(boost_source_dir "${boost_${boost_lib}_SOURCE_DIR}")
#   if(NOT boost_source_dir)
#     message(FATAL_ERROR "CPM failed to fetch boostorg/${boost_lib}")
#   endif()

#   target_include_directories(${PROJECT_NAME} PRIVATE ${boost_source_dir}/include)
# endfunction()

# foreach(boost_lib IN LISTS BASELIB_BOOST_LIBRARIES)
#   baselib_add_boost_library(${boost_lib})
# endforeach()
