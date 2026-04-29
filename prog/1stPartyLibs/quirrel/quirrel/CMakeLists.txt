cmake_minimum_required(VERSION 3.5)
project(squirrel VERSION 4.0 LANGUAGES C CXX)

option(ENABLE_VAR_TRACE "Enable variable change tracing feature.")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif ()

include(GNUInstallDirs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_CXX_STANDARD 17)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(
    "$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti;-fno-exceptions>"
    -fno-strict-aliasing
    -Wall
    -Wextra
    -pedantic
    -Wcast-qual
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    "$<$<CONFIG:Release>:-O3>"
    "$<$<CONFIG:RelWithDebInfo>:-O3;-g>"
    "$<$<CONFIG:MinSizeRel>:-Os>"
  )
  # Debug flags
  if(CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options("$<$<CONFIG:Debug>:-g3;-Og>")
  else()
    # Clang debug flags
    add_compile_options("$<$<CONFIG:Debug>:-g;-Og>")
  endif()
elseif(MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_compile_options(/wd4244 /wd4018 /wd4267) # disable type conversion warnings
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  add_compile_definitions(_SQ64)
endif()

add_subdirectory(squirrel)
add_subdirectory(squirrel/compiler)
add_subdirectory(sqstdlib)
add_subdirectory(sqmodules)
add_subdirectory(sq)


include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/cmake/squirrel/squirrel-config-version.cmake"
  VERSION "${squirrel_VERSION}"
  COMPATIBILITY AnyNewerVersion
  )

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/squirrel-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/cmake/squirrel/squirrel-config.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/squirrel"
  )
