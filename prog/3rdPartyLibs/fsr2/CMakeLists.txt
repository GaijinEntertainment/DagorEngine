# This file is part of the FidelityFX SDK.
#
# Copyright (c) 2022-2023 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

cmake_minimum_required(VERSION 3.15)
set(CMAKE_DEBUG_POSTFIX d)

option (FFX_FSR2_API_DX12 "Build FSR 2.0 DX12 backend" ON)
option (FFX_FSR2_API_VK "Build FSR 2.0 Vulkan backend" ON)

set(FSR2_AUTO_COMPILE_SHADERS ON CACHE BOOL "Compile shaders automatically as a prebuild step.")

if(CMAKE_GENERATOR STREQUAL "Ninja")
    set(USE_DEPFILE TRUE)
else()
    set(USE_DEPFILE FALSE)
endif()

if(CMAKE_GENERATOR STREQUAL "Visual Studio 16 2019")
    set(FSR2_VS_VERSION 2019)
endif()

if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64" OR CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:x64")
    set(FSR2_PLATFORM_NAME x64)
elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32" OR CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:X86")
    set(FSR2_PLATFORM_NAME x86)
else()
    message(FATAL_ERROR "Unsupported target platform - only supporting x64 and Win32 currently")
endif()

# Embed PDBs in the debug versions of the libs
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Z7")

# Write both debug and release versions of the static libs to the /lib folder as they are uniquely named
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_HOME_DIRECTORY}/bin/ffx_fsr2_api/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_HOME_DIRECTORY}/bin/ffx_fsr2_api/)

add_compile_definitions(_UNICODE)
add_compile_definitions(UNICODE)
#add_compile_definitions(FSR2_VERSION_MAJOR=0)
#add_compile_definitions(FSR2_VERSION_MINOR=1)
#add_compile_definitions(FSR2_VERSION_PATCH=0)

if(FSR2_VS_VERSION STREQUAL 2015)
    message(NOTICE "Forcing the SDK path for VS 2015")
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0.18362.0")
endif()

set(FFX_SC_EXECUTABLE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../tools/sc/FidelityFX_SC.exe)

set(FFX_SC_BASE_ARGS
    -reflection -deps=gcc -DFFX_GPU=1
    # Only reprojection is to do half for now
    -DFFX_FSR2_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF=0
    -DFFX_FSR2_OPTION_ACCUMULATE_SAMPLERS_USE_DATA_HALF=0
    -DFFX_FSR2_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF=1
    -DFFX_FSR2_OPTION_POSTPROCESSLOCKSTATUS_SAMPLERS_USE_DATA_HALF=0
    # Upsample uses lanczos approximation
    -DFFX_FSR2_OPTION_UPSAMPLE_USE_LANCZOS_TYPE=2
    )

set(FFX_SC_PERMUTATION_ARGS
    # Reproject can use either reference lanczos or LUT
    -DFFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE={0,1}
    -DFFX_FSR2_OPTION_HDR_COLOR_INPUT={0,1}
    -DFFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS={0,1}
    -DFFX_FSR2_OPTION_JITTERED_MOTION_VECTORS={0,1}
    -DFFX_FSR2_OPTION_INVERTED_DEPTH={0,1}
    -DFFX_FSR2_OPTION_APPLY_SHARPENING={0,1})
 
file(GLOB SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

if (FSR2_BUILD_AS_DLL)
    add_library(ffx_fsr2_api_${FSR2_PLATFORM_NAME}      SHARED ${SOURCES})
else()
    add_library(ffx_fsr2_api_${FSR2_PLATFORM_NAME}      STATIC ${SOURCES})
endif()

# graphics api backends
if(FFX_FSR2_API_DX12)
    message("Will build FSR2 library: DX12 backend")
    add_subdirectory(dx12)
endif()
if(FFX_FSR2_API_VK)
    message("Will build FSR2 library: Vulkan backend")
    add_subdirectory(vk)
endif()

# api
source_group("source"  FILES ${SOURCES})

set_source_files_properties(${SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)
