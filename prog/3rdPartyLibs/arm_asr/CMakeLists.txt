# Remove standalone VK backend
set(FFXM_REMOVE_ARM_ASR_VK_STANDALONE_BACKEND ON CACHE BOOL "Compile away the VK standalone backend.")
# Use GLSL shaders
set(FFXM_USE_GLSL_SHADERS OFF CACHE BOOL "Use GLSL shaders instead of HLSL shaders.")

if(CMAKE_GENERATOR STREQUAL "Ninja")
    set(USE_DEPFILE TRUE)
else()
    set(USE_DEPFILE FALSE)
endif()

# Set Visual Studio version
if(CMAKE_GENERATOR STREQUAL "Visual Studio 14 2015")
    set(FFXM_VS_VERSION 2015)
elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 15 2017")
	set(FFXM_VS_VERSION 2017)
elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 16 2019")
    set(FFXM_VS_VERSION 2019)
elseif(CMAKE_GENERATOR STREQUAL "Visual Studio 17 2022")
	set(FFXM_VS_VERSION 2022)
endif()

# Add requred compile definitions
add_compile_definitions(_UNICODE)
add_compile_definitions(UNICODE)

if(FFXM_VS_VERSION STREQUAL 2015 OR FFXM_VS_VERSION STREQUAL 2017)
    message(NOTICE "Forcing the SDK path for VS 2015 and VS 2017")
    set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0.18362.0")
endif()

# Setup common variables
set(FFXM_SC_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/tools/bin/FidelityFX_SC.exe)
set(FFXM_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(FFXM_SHARED_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/shared)
set(FFXM_HOST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include/host)
set(FFXM_GPU_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include/gpu)
set(FFXM_COMPONENTS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/components)
set(FFXM_HOST_BACKENDS_PATH ${FFXM_HOST_PATH}/backends)
set(FFXM_SRC_BACKENDS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/backends)

# ------------------------------------------------------
# Define common source groupings used by all effects

# Set shared and global public sources
# file(GLOB FFXM_SHARED_SOURCES
#     "${FFXM_SHARED_PATH}/*.cpp"
#     "${FFXM_SHARED_PATH}/*.h")

# set(FFXM_PUBLIC_SOURCES
#     "${FFXM_HOST_PATH}/ffxm_assert.h"
#     "${FFXM_HOST_PATH}/ffxm_error.h"
# 	"${FFXM_HOST_PATH}/ffxm_interface.h"
#     "${FFXM_HOST_PATH}/ffxm_types.h"
#     "${FFXM_HOST_PATH}/ffxm_util.h")

# Init shader sources (for easy viewing)
set(FFXM_PUBLIC_SHADER_SOURCES
    "${FFXM_GPU_PATH}/ffx_common_types.h"
    "${FFXM_GPU_PATH}/ffx_core.h"
    "${FFXM_GPU_PATH}/ffx_core_cpu.h"
    "${FFXM_GPU_PATH}/ffx_core_glsl.h"
    "${FFXM_GPU_PATH}/ffx_core_gpu_common.h"
    "${FFXM_GPU_PATH}/ffx_core_gpu_common_half.h"
    "${FFXM_GPU_PATH}/ffx_core_hlsl.h"
    "${FFXM_GPU_PATH}/ffx_core_portability.h")

# Components
if(NOT FFXM_REMOVE_ARM_ASR_VK_STANDALONE_BACKEND)
add_subdirectory(${FFXM_SRC_BACKENDS_PATH}/vk)
endif()

set(SRC "${FFXM_SHARED_PATH}/ffxm_assert.cpp")
list(APPEND SRC "${FFXM_SHARED_PATH}/ffxm_object_management.cpp")
list(APPEND SRC "${FFXM_COMPONENTS_PATH}/fsr2/ffxm_fsr2.cpp")

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/host/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/host/backends/vk)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/gpu/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)
if(NOT MSVC)
	add_compile_options(-std=c++20)
else()
	add_compile_options(/std:c++20)
endif()

add_library(Arm_ASR_api ${SRC})
