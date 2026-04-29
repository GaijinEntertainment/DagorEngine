###########################################################
# C++ Integration Tutorials — standalone build against
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
project(daslang_cpp_tutorials CXX)

find_package(DAS REQUIRED)
find_package(Threads REQUIRED)

# Resolve the SDK root (two levels up from lib/cmake/DAS/).
get_filename_component(DAS_SDK_ROOT "${DAS_DIR}/../../.." ABSOLUTE)
message(STATUS "daslang SDK root: ${DAS_SDK_ROOT}")

# Put executables into the SDK bin/ directory so getDasRoot()
# finds daslib/, tutorials/, etc. without any source changes.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${DAS_SDK_ROOT}/bin")
# Multi-config generators (Visual Studio) append a per-config subdirectory.
# Force all configs into the same bin/ folder.
foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} "${DAS_SDK_ROOT}/bin")
endforeach()

# Source directory (where the tutorial .cpp/.das files live).
set(TUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

###########################################################
# Helper macro — standard C++ integration tutorial target.
###########################################################
macro(DAS_CPP_TUTORIAL name)
    add_executable(${name} ${ARGN})
    target_compile_definitions(${name} PRIVATE DAS_MOD_EXPORTS)
    target_link_libraries(${name} PRIVATE DAS::libDaScriptDyn Threads::Threads)
    target_compile_features(${name} PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_options(${name} PRIVATE /wd4005)
    endif()
endmacro()

###########################################################
# Helper: AOT code generation.
#
# Runs:  daslang <sdk>/utils/aot/main.das -- <mode> <input> <output>
#
# mode is -aot (generates .cpp) or -ctx (generates dir/ with .h + .cpp)
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
# Simple tutorials (01–12, 14–18, 21–22)
###########################################################
DAS_CPP_TUTORIAL(integration_cpp_01 "${TUT_DIR}/01_hello_world.cpp")
DAS_CPP_TUTORIAL(integration_cpp_02 "${TUT_DIR}/02_calling_functions.cpp")
DAS_CPP_TUTORIAL(integration_cpp_03 "${TUT_DIR}/03_binding_functions.cpp")
DAS_CPP_TUTORIAL(integration_cpp_04 "${TUT_DIR}/04_binding_types.cpp")
DAS_CPP_TUTORIAL(integration_cpp_05 "${TUT_DIR}/05_binding_enums.cpp")
DAS_CPP_TUTORIAL(integration_cpp_06 "${TUT_DIR}/06_interop.cpp")
DAS_CPP_TUTORIAL(integration_cpp_07 "${TUT_DIR}/07_callbacks.cpp")
DAS_CPP_TUTORIAL(integration_cpp_08 "${TUT_DIR}/08_methods.cpp")
DAS_CPP_TUTORIAL(integration_cpp_09 "${TUT_DIR}/09_operators_and_properties.cpp")
DAS_CPP_TUTORIAL(integration_cpp_10 "${TUT_DIR}/10_custom_modules.cpp")
DAS_CPP_TUTORIAL(integration_cpp_11 "${TUT_DIR}/11_context_variables.cpp")
DAS_CPP_TUTORIAL(integration_cpp_12 "${TUT_DIR}/12_smart_pointers.cpp")

DAS_CPP_TUTORIAL(integration_cpp_14 "${TUT_DIR}/14_serialization.cpp")
DAS_CPP_TUTORIAL(integration_cpp_15 "${TUT_DIR}/15_custom_annotations.cpp")
DAS_CPP_TUTORIAL(integration_cpp_16 "${TUT_DIR}/16_sandbox.cpp")
DAS_CPP_TUTORIAL(integration_cpp_17 "${TUT_DIR}/17_coroutines.cpp")
DAS_CPP_TUTORIAL(integration_cpp_18 "${TUT_DIR}/18_dynamic_scripts.cpp")

DAS_CPP_TUTORIAL(integration_cpp_21 "${TUT_DIR}/21_threading.cpp")
DAS_CPP_TUTORIAL(integration_cpp_22 "${TUT_DIR}/22_namespace_integration.cpp")

###########################################################
# Tutorial 13 — AOT
###########################################################
set(AOT_13_SRC)
DAS_TUTORIAL_AOT("${TUT_DIR}/13_aot.das" -aot AOT_13_SRC integration_cpp_13)

DAS_CPP_TUTORIAL(integration_cpp_13
    "${TUT_DIR}/13_aot.cpp"
    "${AOT_13_SRC}"
)

###########################################################
# Tutorial 19 — Class Adapters
#
# Requires module_builtin_rtti.h (installed to
# <sdk>/include/daScript/builtin/) and the pre-generated
# .das.inc and _gen.inc files.
###########################################################
DAS_CPP_TUTORIAL(integration_cpp_19
    "${TUT_DIR}/19_class_adapters.cpp"
)
target_include_directories(integration_cpp_19 PRIVATE
    "${DAS_SDK_ROOT}/include/daScript/builtin"
)

###########################################################
# Tutorial 20 — Standalone Context
###########################################################
set(AOT_20_SRC)
DAS_TUTORIAL_AOT("${TUT_DIR}/standalone_context.das" -ctx AOT_20_SRC integration_cpp_20)

# The -ctx generator also produces a .h header in the output dir.
DAS_CPP_TUTORIAL(integration_cpp_20
    "${TUT_DIR}/20_standalone_context.cpp"
    "${AOT_20_SRC}"
)
target_include_directories(integration_cpp_20 PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
)
