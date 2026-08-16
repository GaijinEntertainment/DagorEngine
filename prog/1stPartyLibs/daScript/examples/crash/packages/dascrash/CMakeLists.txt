cmake_minimum_required(VERSION 3.16)
project(dasModuleCrash CXX)

# DASLANG_DIR must point to the daslang SDK root
if(NOT DEFINED DASLANG_DIR)
    message(FATAL_ERROR "DASLANG_DIR must be set to the daslang SDK root")
endif()

add_library(dasModuleCrash SHARED
    src/module_crash.cpp
)

target_compile_features(dasModuleCrash PRIVATE cxx_std_17)
target_compile_definitions(dasModuleCrash PRIVATE DAS_MOD_EXPORTS DAS_ENABLE_DLL=1
    $<$<CONFIG:Release>:DAS_NO_ASSERTIONS>
    $<$<CONFIG:MinSizeRel>:DAS_NO_ASSERTIONS>
    $<$<CONFIG:RelWithDebInfo>:DAS_NO_ASSERTIONS>
)

target_include_directories(dasModuleCrash PRIVATE
    "${DASLANG_DIR}/include"
)

if(WIN32)
    if(EXISTS "${DASLANG_DIR}/lib/Release/libDaScriptDyn.lib")
        target_link_libraries(dasModuleCrash PRIVATE
            "${DASLANG_DIR}/lib/Release/libDaScriptDyn.lib"
            "${DASLANG_DIR}/lib/Release/libDaScriptDyn_runtime.lib"
        )
    else()
        target_link_libraries(dasModuleCrash PRIVATE
            "${DASLANG_DIR}/lib/libDaScriptDyn.lib"
            "${DASLANG_DIR}/lib/libDaScriptDyn_runtime.lib"
        )
    endif()
else()
    target_link_libraries(dasModuleCrash PRIVATE
        "${DASLANG_DIR}/lib/libDaScriptDyn.so"
        "${DASLANG_DIR}/lib/libDaScriptDyn_runtime.so"
    )
endif()

if(MSVC)
    # /EHsc matches the daslang runtime (CMakeLists.txt:1072). Without it MSVC assumes
    # nothing throws, and an exception escaping this module fast-fails with no diagnostic
    # instead of reaching std::terminate.
    target_compile_options(dasModuleCrash PRIVATE /wd4005 /EHsc)
endif()

set_target_properties(dasModuleCrash PROPERTIES
    SUFFIX ".shared_module"
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
