cmake_minimum_required(VERSION 3.16)
project(dasModuleCounter CXX)

# DASLANG_DIR must point to the daslang SDK root
if(NOT DEFINED DASLANG_DIR)
    message(FATAL_ERROR "DASLANG_DIR must be set to the daslang SDK root")
endif()

add_library(dasModuleCounter SHARED
    src/module_counter.cpp
)

target_compile_features(dasModuleCounter PRIVATE cxx_std_17)
target_compile_definitions(dasModuleCounter PRIVATE DAS_MOD_EXPORTS DAS_ENABLE_DLL=1
    $<$<CONFIG:Release>:DAS_NO_ASSERTIONS>
    $<$<CONFIG:MinSizeRel>:DAS_NO_ASSERTIONS>
    $<$<CONFIG:RelWithDebInfo>:DAS_NO_ASSERTIONS>
)

target_include_directories(dasModuleCounter PRIVATE
    ${DASLANG_DIR}/include
)

if(WIN32)
    target_link_libraries(dasModuleCounter PRIVATE
        ${DASLANG_DIR}/lib/Release/libDaScriptDyn.lib
    )
else()
    target_link_libraries(dasModuleCounter PRIVATE
        ${DASLANG_DIR}/lib/libDaScriptDyn.so
    )
endif()

if(MSVC)
    target_compile_options(dasModuleCounter PRIVATE /wd4005)
endif()

set_target_properties(dasModuleCounter PROPERTIES
    SUFFIX ".shared_module"
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
