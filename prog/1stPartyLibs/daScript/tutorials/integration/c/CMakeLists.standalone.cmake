###########################################################
# C Integration Tutorials — standalone build against
# an installed daslang SDK.
#
# Usage:
#   cmake -DCMAKE_PREFIX_PATH=<sdk-root> <this-directory>
#   cmake --build . --config Release
#
# Example (Windows):
#   mkdir build && cd build
#   cmake -DCMAKE_PREFIX_PATH=D:/daslang ..
#   cmake --build . --config Release
#
# The built executables are placed into <sdk-root>/bin/ so
# that getDasRoot() automatically resolves the SDK root at
# runtime and finds daslib/, tutorials/, etc.
###########################################################

cmake_minimum_required(VERSION 3.16)
project(daslang_c_tutorials C CXX)

find_package(DAS REQUIRED)
find_package(Threads REQUIRED)

# Resolve the SDK root (two levels up from lib/cmake/DAS/).
get_filename_component(DAS_SDK_ROOT "${DAS_DIR}/../../.." ABSOLUTE)
message(STATUS "daslang SDK root: ${DAS_SDK_ROOT}")

# Put executables into the SDK bin/ directory so getDasRoot()
# finds daslib/, tutorials/, etc. without any source changes.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${DAS_SDK_ROOT}/bin")
foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} "${DAS_SDK_ROOT}/bin")
endforeach()

# Source directory (where the tutorial .c/.das files live).
set(TUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# The C++ integration tutorials directory (needed for 13_aot.das
# which is shared with C tutorial 09).
get_filename_component(CPP_TUT_DIR "${TUT_DIR}/../cpp" ABSOLUTE)

###########################################################
# Helper macro — standard C integration tutorial target.
# C source is compiled as C but linked with C++ (the daScript
# runtime is C++).
###########################################################
macro(DAS_C_TUTORIAL name)
    add_executable(${name} ${ARGN})
    target_compile_definitions(${name} PRIVATE DAS_MOD_EXPORTS)
    target_link_libraries(${name} PRIVATE DAS::libDaScriptDyn Threads::Threads)
    set_target_properties(${name} PROPERTIES
        LINKER_LANGUAGE CXX
        CXX_STANDARD 17
    )
    if(MSVC)
        target_compile_options(${name} PRIVATE /wd4005)
    endif()
endmacro()

###########################################################
# Helper: AOT code generation (same as C++ variant).
###########################################################
macro(DAS_TUTORIAL_AOT input mode out_var target_name)
    get_filename_component(_abs_input "${input}" ABSOLUTE)
    get_filename_component(_input_name "${input}" NAME)
    if("${mode}" STREQUAL "-ctx")
        set(_out_dir "${CMAKE_CURRENT_BINARY_DIR}/_standalone_ctx_generated")
        set(_out_arg "${_out_dir}/")
        set(_out_src "${_out_dir}/${_input_name}.cpp")
    else()
        set(_out_dir "${CMAKE_CURRENT_BINARY_DIR}/_aot_generated")
        set(_out_arg "${_out_dir}/${target_name}_${_input_name}.cpp")
        set(_out_src "${_out_dir}/${target_name}_${_input_name}.cpp")
    endif()
    file(MAKE_DIRECTORY "${_out_dir}")
    add_custom_command(
        OUTPUT  "${_out_src}"
        DEPENDS "${_abs_input}"
        COMMENT "AOT: ${_input_name} (${mode})"
        COMMAND $<TARGET_FILE:DAS::daslang>
                "${DAS_SDK_ROOT}/utils/aot/main.das"
                -- ${mode} "${_abs_input}" "${_out_arg}"
    )
    set(${out_var} "${_out_src}")
    set_source_files_properties("${_out_src}" PROPERTIES GENERATED TRUE)
endmacro()

###########################################################
# Simple tutorials (01–08, 10)
###########################################################
DAS_C_TUTORIAL(integration_c_01 "${TUT_DIR}/01_hello_world.c")
DAS_C_TUTORIAL(integration_c_02 "${TUT_DIR}/02_calling_functions.c")
DAS_C_TUTORIAL(integration_c_03 "${TUT_DIR}/03_binding_types.c")
DAS_C_TUTORIAL(integration_c_04 "${TUT_DIR}/04_callbacks.c")
DAS_C_TUTORIAL(integration_c_05 "${TUT_DIR}/05_unaligned_advanced.c")
DAS_C_TUTORIAL(integration_c_06 "${TUT_DIR}/06_sandbox.c")
DAS_C_TUTORIAL(integration_c_07 "${TUT_DIR}/07_context_variables.c")
DAS_C_TUTORIAL(integration_c_08 "${TUT_DIR}/08_serialization.c")
DAS_C_TUTORIAL(integration_c_10 "${TUT_DIR}/10_threading.c")

###########################################################
# Tutorial 09 — AOT (C)
#
# Reuses the same 13_aot.das script from the C++ tutorials.
###########################################################
set(AOT_C09_SRC)
DAS_TUTORIAL_AOT("${CPP_TUT_DIR}/13_aot.das" -aot AOT_C09_SRC integration_c_09)

DAS_C_TUTORIAL(integration_c_09
    "${TUT_DIR}/09_aot.c"
    "${AOT_C09_SRC}"
)
