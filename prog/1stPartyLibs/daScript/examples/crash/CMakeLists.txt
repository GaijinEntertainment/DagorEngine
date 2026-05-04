add_executable(example_crash
    ${CMAKE_CURRENT_SOURCE_DIR}/examples/crash/main.cpp
)
TARGET_COMPILE_DEFINITIONS(example_crash PRIVATE DAS_MOD_EXPORTS)
TARGET_LINK_LIBRARIES(example_crash libDaScriptDyn Threads::Threads)
ADD_DEPENDENCIES(example_crash libDaScriptDyn)
SETUP_CPP11(example_crash)
