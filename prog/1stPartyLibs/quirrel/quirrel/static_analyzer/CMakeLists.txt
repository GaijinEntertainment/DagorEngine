cmake_minimum_required(VERSION 3.4)
project(quirrel_static_analyzer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif ()

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(EXECUTABLE_OUTPUT_PATH bin)

set(SOURCE
  compilation_context.cpp
  module_exports.cpp
  quirrel_lexer.cpp
  quirrel_parser.cpp
  quirrel_static_analyzer.cpp
  json_output.cpp
)

add_executable(quirrel_static_analyzer ${SOURCE})

TARGET_INCLUDE_DIRECTORIES(quirrel_static_analyzer PUBLIC
  ${CMAKE_SOURCE_DIR} # for importParser
)
