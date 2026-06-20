cmake_minimum_required(VERSION 3.16)
project(dasModuleFastMath C)

# DASLANG_DIR must point to the daslang SDK root
if(NOT DEFINED DASLANG_DIR)
    message(FATAL_ERROR "DASLANG_DIR must be set to the daslang SDK root")
endif()

add_library(dasModuleFastMath SHARED
    src/fastmath.c
)

target_compile_definitions(dasModuleFastMath PRIVATE DAS_ENABLE_DLL=1)

target_include_directories(dasModuleFastMath PRIVATE
    "${DASLANG_DIR}/include"
)

find_library(DASCRIPT_DYN_LIB
    NAMES libDaScriptDyn DaScriptDyn
    PATHS "${DASLANG_DIR}/lib/Release" "${DASLANG_DIR}/lib"
    NO_DEFAULT_PATH
    REQUIRED
)
target_link_libraries(dasModuleFastMath PRIVATE ${DASCRIPT_DYN_LIB})

set_target_properties(dasModuleFastMath PROPERTIES
    SUFFIX ".shared_module"
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
