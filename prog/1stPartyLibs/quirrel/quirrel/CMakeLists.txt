cmake_minimum_required(VERSION 3.4)
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

if(CMAKE_COMPILER_IS_GNUCXX)
  add_compile_options(
    "$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti;-fno-exceptions>"
    -fno-strict-aliasing
    -Wall
    -Wextra
    -pedantic
    -Wcast-qual
    "$<$<CONFIG:Release>:-O3>"
    "$<$<CONFIG:RelWithDebInfo>:-O3;-g>"
    "$<$<CONFIG:MinSizeRel>:-Os>"
    "$<$<CONFIG:Debug>:-pg;-pie;-gstabs;-g3;-Og>"
    )
elseif(MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(squirrel)
add_subdirectory(sqstdlib)
add_subdirectory(sqmodules)
add_subdirectory(sq)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(tgts)
  list(APPEND tgts squirrel sqstdlib sqmodules sq)
  foreach(t ${tgts})
    target_compile_definitions(${t} PUBLIC -D_SQ64)
  endforeach()
endif()


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
