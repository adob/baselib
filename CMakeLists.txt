cmake_minimum_required(VERSION 3.16)

get_filename_component(
  CURRENT_DIRNAME
  ${CMAKE_CURRENT_LIST_DIR}
  NAME
)

project(baselib VERSION 0.0.0 LANGUAGES CXX)

include(cmake/CPM.cmake)

set(BASELIB_BOOST_VERSION "1.87.0" CACHE STRING "Boost version used by baselib CPM dependencies")
set(BASELIB_BOOST_COMPONENTS "atomic;circular_buffer" CACHE STRING "Boost components required by baselib")


#set(CPM_USE_LOCAL_PACKAGES ON)

CPMFindPackage(
  NAME Boost
  VERSION ${BASELIB_BOOST_VERSION} # Versions less than 1.85.0 may need patches for installation targets.
  URL https://github.com/boostorg/boost/releases/download/boost-${BASELIB_BOOST_VERSION}/boost-${BASELIB_BOOST_VERSION}-cmake.tar.xz
  URL_HASH SHA256=7da75f171837577a52bbf217e17f8ea576c7c246e4594d617bfde7fafd408be5
  FIND_PACKAGE_ARGUMENTS "CONFIG;COMPONENTS;${BASELIB_BOOST_COMPONENTS}"
  OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_SKIP_INSTALL_RULES ON" # Set `OFF` for installation
          "BUILD_SHARED_LIBS OFF"
)

CPMFindPackage(
  NAME fmt
  VERSION 11.1.4
  GITHUB_REPOSITORY fmtlib/fmt
  GIT_TAG 11.1.4
)

# CPMFindPackage(
#   NAME cpptrace
#   VERSION 0.8.2
#   GITHUB_REPOSITORY jeremy-rifkin/cpptrace
#   GIT_TAG v0.8.2
# )


file(GLOB SOURCES 
    ${CMAKE_CURRENT_LIST_DIR}/lib/*.cc
    #${CMAKE_CURRENT_LIST_DIR}/lib/debug/*.cc
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

    #${CMAKE_CURRENT_LIST_DIR}/deps/fmt/src/format.cc
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

set(BASELIB_BOOST_TARGETS Boost::headers)
foreach(boost_component IN LISTS BASELIB_BOOST_COMPONENTS)
  if(TARGET "Boost::${boost_component}")
    list(APPEND BASELIB_BOOST_TARGETS "Boost::${boost_component}")
  endif()
endforeach()

target_link_libraries(${PROJECT_NAME} PUBLIC ${BASELIB_BOOST_TARGETS})
target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt)
#target_link_libraries(${PROJECT_NAME} PUBLIC cpptrace::cpptrace)
