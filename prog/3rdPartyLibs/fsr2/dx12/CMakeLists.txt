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

if(NOT ${FFX_FSR2_API_DX12})
    return()
endif()

set(FFX_SC_DX12_BASE_ARGS
    -E CS -Wno-for-redefinition -Wno-ambig-lit-shift -DFFX_HLSL=1 -DFFX_HLSL_6_2=1)

file(GLOB SHADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/../shaders/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../shaders/*.hlsl")

set(PASS_SHADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/ffx_fsr2_tcr_autogen_pass.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/ffx_fsr2_autogen_reactive_pass.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/ffx_fsr2_accumulate_pass.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/ffx_fsr2_compute_luminance_pyramid_pass.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/ffx_fsr2_depth_clip_pass.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/ffx_fsr2_lock_pass.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/ffx_fsr2_reconstruct_previous_depth_pass.hlsl
    ${CMAKE_CURRENT_SOURCE_DIR}/../shaders/ffx_fsr2_rcas_pass.hlsl)    

file(GLOB_RECURSE DX12
    "${CMAKE_CURRENT_SOURCE_DIR}/../ffx_assert.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

if (FSR2_BUILD_AS_DLL)
    add_library(ffx_fsr2_api_dx12_${FSR2_PLATFORM_NAME} SHARED ${DX12})
else()
    add_library(ffx_fsr2_api_dx12_${FSR2_PLATFORM_NAME} STATIC ${DX12})
endif()

target_include_directories(ffx_fsr2_api_dx12_${FSR2_PLATFORM_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/../shaders/dx12)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../shaders/dx12)

if (FSR2_AUTO_COMPILE_SHADERS)
    set(FFX_SC_DEPENDENT_TARGET ffx_fsr2_api_dx12_${FSR2_PLATFORM_NAME})
else()
    set(FFX_SC_DEPENDENT_TARGET ffx_fsr2_api_dx12_shaders_${FSR2_PLATFORM_NAME})
    add_custom_target(${FFX_SC_DEPENDENT_TARGET})
endif()

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20.0") 
cmake_policy(SET CMP0116 OLD)
endif()
get_filename_component(PASS_SHADER_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/../shaders/dx12 ABSOLUTE)
foreach(PASS_SHADER ${PASS_SHADERS})
    get_filename_component(PASS_SHADER_FILENAME ${PASS_SHADER} NAME_WE)
    get_filename_component(PASS_SHADER_TARGET ${PASS_SHADER} NAME_WLE)
    set(WAVE32_PERMUTATION_HEADER ${PASS_SHADER_OUTPUT_PATH}/${PASS_SHADER_TARGET}_permutations.h)
    set(WAVE64_PERMUTATION_HEADER ${PASS_SHADER_OUTPUT_PATH}/${PASS_SHADER_TARGET}_wave64_permutations.h)
    set(WAVE32_16BIT_PERMUTATION_HEADER ${PASS_SHADER_OUTPUT_PATH}/${PASS_SHADER_TARGET}_16bit_permutations.h)
    set(WAVE64_16BIT_PERMUTATION_HEADER ${PASS_SHADER_OUTPUT_PATH}/${PASS_SHADER_TARGET}_wave64_16bit_permutations.h)

    # combine base and permutation args
    set(FFX_SC_ARGS ${FFX_SC_BASE_ARGS} ${FFX_SC_DX12_BASE_ARGS} ${FFX_SC_PERMUTATION_ARGS})

    if (USE_DEPFILE)
        # Wave32 
        add_custom_command(
            OUTPUT ${WAVE32_PERMUTATION_HEADER}
            COMMAND ${FFX_SC_EXECUTABLE} ${FFX_SC_ARGS} -name=${PASS_SHADER_FILENAME} -DFFX_HALF=0 -T cs_6_2 -I ${CMAKE_CURRENT_SOURCE_DIR}/../shaders -output=${PASS_SHADER_OUTPUT_PATH} ${PASS_SHADER}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS ${PASS_SHADER}
            DEPFILE ${WAVE32_PERMUTATION_HEADER}.d
        )
        list(APPEND PERMUTATION_OUTPUTS ${WAVE32_PERMUTATION_HEADER})

        # Wave64
        add_custom_command(
            OUTPUT ${WAVE64_PERMUTATION_HEADER}
            COMMAND ${FFX_SC_EXECUTABLE} ${FFX_SC_ARGS} -name=${PASS_SHADER_FILENAME}_wave64 "-DFFX_FSR2_PREFER_WAVE64=\"[WaveSize(64)]\"" -DFFX_HALF=0 -T cs_6_6 -I ${CMAKE_CURRENT_SOURCE_DIR}/../shaders -output=${PASS_SHADER_OUTPUT_PATH} ${PASS_SHADER}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS ${PASS_SHADER}
            DEPFILE ${WAVE64_PERMUTATION_HEADER}.d
        )
        list(APPEND PERMUTATION_OUTPUTS ${WAVE64_PERMUTATION_HEADER})

        # skip 16-bit permutations for the compute luminance pyramid pass
        if (NOT ${PASS_SHADER_FILENAME} STREQUAL "ffx_fsr2_compute_luminance_pyramid_pass")
            # Wave32 16-bit
            add_custom_command(
                OUTPUT ${WAVE32_16BIT_PERMUTATION_HEADER}
                COMMAND ${FFX_SC_EXECUTABLE} ${FFX_SC_ARGS} -name=${PASS_SHADER_FILENAME}_16bit -DFFX_HALF=1 -enable-16bit-types -T cs_6_2 -I ${CMAKE_CURRENT_SOURCE_DIR}/../shaders -output=${PASS_SHADER_OUTPUT_PATH} ${PASS_SHADER}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS ${PASS_SHADER}
                DEPFILE ${WAVE32_16BIT_PERMUTATION_HEADER}.d
            )
            list(APPEND PERMUTATION_OUTPUTS ${WAVE32_16BIT_PERMUTATION_HEADER})

            # Wave64 16-bit
            add_custom_command(
                OUTPUT ${WAVE64_16BIT_PERMUTATION_HEADER}
                COMMAND ${FFX_SC_EXECUTABLE} ${FFX_SC_ARGS} -name=${PASS_SHADER_FILENAME}_wave64_16bit "-DFFX_FSR2_PREFER_WAVE64=\"[WaveSize(64)]\"" -DFFX_HALF=1 -enable-16bit-types -T cs_6_6 -I ${CMAKE_CURRENT_SOURCE_DIR}/../shaders -output=${PASS_SHADER_OUTPUT_PATH} ${PASS_SHADER}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS ${PASS_SHADER}
                DEPFILE ${WAVE64_16BIT_PERMUTATION_HEADER}.d
            )
            list(APPEND PERMUTATION_OUTPUTS ${WAVE64_16BIT_PERMUTATION_HEADER})
        endif()
    else()
        # Wave32 
        add_custom_command(
            OUTPUT ${WAVE32_PERMUTATION_HEADER}
            COMMAND ${FFX_SC_EXECUTABLE} ${FFX_SC_ARGS} -name=${PASS_SHADER_FILENAME} -DFFX_HALF=0 -T cs_6_2 -I ${CMAKE_CURRENT_SOURCE_DIR}/../shaders -output=${PASS_SHADER_OUTPUT_PATH} ${PASS_SHADER}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS ${PASS_SHADER}
        )
        list(APPEND PERMUTATION_OUTPUTS ${WAVE32_PERMUTATION_HEADER})

        # Wave64
        add_custom_command(
            OUTPUT ${WAVE64_PERMUTATION_HEADER}
            COMMAND ${FFX_SC_EXECUTABLE} ${FFX_SC_ARGS} -name=${PASS_SHADER_FILENAME}_wave64 "-DFFX_FSR2_PREFER_WAVE64=\"[WaveSize(64)]\"" -DFFX_HALF=0 -T cs_6_6 -I ${CMAKE_CURRENT_SOURCE_DIR}/../shaders -output=${PASS_SHADER_OUTPUT_PATH} ${PASS_SHADER}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS ${PASS_SHADER}
        )
        list(APPEND PERMUTATION_OUTPUTS ${WAVE64_PERMUTATION_HEADER})

        # Skip 16-bit permutations for the compute luminance pyramid pass
        if (NOT ${PASS_SHADER_FILENAME} STREQUAL "ffx_fsr2_compute_luminance_pyramid_pass")
            # Wave32 16-bit
            add_custom_command(
                OUTPUT ${WAVE32_16BIT_PERMUTATION_HEADER}
                COMMAND ${FFX_SC_EXECUTABLE} ${FFX_SC_ARGS} -name=${PASS_SHADER_FILENAME}_16bit -DFFX_HALF=1 -enable-16bit-types -T cs_6_2 -I ${CMAKE_CURRENT_SOURCE_DIR}/../shaders -output=${PASS_SHADER_OUTPUT_PATH} ${PASS_SHADER}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS ${PASS_SHADER}
            )
            list(APPEND PERMUTATION_OUTPUTS ${WAVE32_16BIT_PERMUTATION_HEADER})

            # Wave64 16-bit
            add_custom_command(
                OUTPUT ${WAVE64_16BIT_PERMUTATION_HEADER}
                COMMAND ${FFX_SC_EXECUTABLE} ${FFX_SC_ARGS} -name=${PASS_SHADER_FILENAME}_wave64_16bit "-DFFX_FSR2_PREFER_WAVE64=\"[WaveSize(64)]\"" -DFFX_HALF=1 -enable-16bit-types -T cs_6_6 -I ${CMAKE_CURRENT_SOURCE_DIR}/../shaders -output=${PASS_SHADER_OUTPUT_PATH} ${PASS_SHADER}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                DEPENDS ${PASS_SHADER}
            )
            list(APPEND PERMUTATION_OUTPUTS ${WAVE64_16BIT_PERMUTATION_HEADER})
        endif()
    endif()
endforeach(PASS_SHADER)

add_custom_target(shader_permutations_dx12 DEPENDS ${PERMUTATION_OUTPUTS})
add_dependencies(${FFX_SC_DEPENDENT_TARGET} shader_permutations_dx12)

source_group("source"  FILES ${DX12})
source_group("shaders" FILES ${SHADERS})