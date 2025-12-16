cmake_minimum_required(VERSION 3.16)

get_filename_component(
  CURRENT_DIRNAME
  ${CMAKE_CURRENT_LIST_DIR}
  NAME
)

project(${CURRENT_DIRNAME} VERSION 0.0.0 LANGUAGES CXX)

file(GLOB SOURCES 
    ${CMAKE_CURRENT_LIST_DIR}/lib/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/errors/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/fmt/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/io/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/os/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/lib/math/*.cc
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

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_26)

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

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/config/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/circular_buffer/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/static_assert/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/type_traits/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/throw_exception/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/assert/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/core/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/move/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/concept_check/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/preprocessor/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/fmt/include/)


#target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../deps/eigen)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/math/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/variant/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/type_index/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/container_hash/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/describe/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/mp11/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/mpl/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/detail/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/utility/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/integer/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/serialization/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/fusion/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/function_types/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/io/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/smart_ptr/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/asio/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/align/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/iostreams/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/range/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/iterator/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/foreach/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/bind/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/optional/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/variant2/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/spirit/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/predef/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/array/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/function/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/filesystem/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/system/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/format/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/property_tree/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/any/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/multi_index/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/deps/boost/libs/tuple/include/)