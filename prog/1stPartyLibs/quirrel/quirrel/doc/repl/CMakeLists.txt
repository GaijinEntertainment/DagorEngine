cmake_minimum_required(VERSION 3.4)
project(sqjsrepl VERSION 4.0 LANGUAGES C CXX)

option(DISABLE_STATIC "Avoid building/installing static libraries." OFF)
option(DISABLE_DYNAMIC "Avoid building/installing dynamic libraries." ON)
option(ENABLE_VAR_TRACE "Enable variable change tracing feature." OFF)

add_subdirectory(../../squirrel "${PROJECT_BINARY_DIR}/squirrel")
add_subdirectory(../../sqstdlib "${PROJECT_BINARY_DIR}/sqstdlib")
add_subdirectory(../../sqmodules "${PROJECT_BINARY_DIR}/sqmodules")

#add_library(sqjsrepl native.cpp)
add_executable(sqjsrepl native.cpp)
target_link_libraries(sqjsrepl squirrel_static sqstdlib_static sqmodules_static)

target_include_directories(squirrel_static PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../../include>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../../internal>"
)

target_include_directories(sqstdlib_static PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../../include>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../../internal>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../..>"
)

target_include_directories(sqmodules_static PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../../include>"
)

target_include_directories(sqjsrepl PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../../include>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/../../sqmodules>"
)

set_target_properties(sqjsrepl PROPERTIES LINK_FLAGS "--bind -sFILESYSTEM=0")

set_target_properties(sqjsrepl PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/static"
)
