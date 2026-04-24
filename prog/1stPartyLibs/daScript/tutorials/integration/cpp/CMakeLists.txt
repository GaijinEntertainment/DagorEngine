###########################################################
# C++ Integration Tutorials — one executable per tutorial
###########################################################

# Helper macro: create a C++ integration tutorial target.
MACRO(DAS_CPP_INTEGRATION_TUTORIAL target_name)
    add_executable(${target_name} ${ARGN})
    TARGET_COMPILE_DEFINITIONS(${target_name} PRIVATE DAS_MOD_EXPORTS)
    TARGET_LINK_LIBRARIES(${target_name} libDaScriptDyn Threads::Threads)
    ADD_DEPENDENCIES(${target_name} libDaScriptDyn)
    SETUP_CPP11(${target_name})
ENDMACRO()

#################################
# Tutorial 1 — Hello World
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_01
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/01_hello_world.cpp
)

#################################
# Tutorial 2 — Calling Functions
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_02
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/02_calling_functions.cpp
)

#################################
# Tutorial 3 — Binding Functions
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_03
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/03_binding_functions.cpp
)

#################################
# Tutorial 4 — Binding Types
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_04
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/04_binding_types.cpp
)

#################################
# Tutorial 5 — Binding Enums
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_05
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/05_binding_enums.cpp
)

#################################
# Tutorial 6 — Low-Level Interop
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_06
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/06_interop.cpp
)

#################################
# Tutorial 7 — Callbacks
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_07
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/07_callbacks.cpp
)

#################################
# Tutorial 8 — Binding Methods
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_08
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/08_methods.cpp
)

#################################
# Tutorial 9 — Operators & Properties
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_09
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/09_operators_and_properties.cpp
)

#################################
# Tutorial 10 — Custom Modules
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_10
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/10_custom_modules.cpp
)

#################################
# Tutorial 11 — Context Variables
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_11
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/11_context_variables.cpp
)

#################################
# Tutorial 12 — Smart Pointers
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_12
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/12_smart_pointers.cpp
)

#################################
# Tutorial 13 — AOT
#################################
# AOT requires a two-stage build:
#   1. Generate C++ from the .das script using daslang's AOT tool
#   2. Compile the generated .cpp into the tutorial executable
SET(INTEGRATION_13_AOT_GENERATED_SRC)
add_custom_target(integration_cpp_13_dasAotStub)
DAS_AOT("tutorials/integration/cpp/13_aot.das"
        INTEGRATION_13_AOT_GENERATED_SRC integration_cpp_13_dasAotStub daslang)

add_executable(integration_cpp_13
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/13_aot.cpp
    ${INTEGRATION_13_AOT_GENERATED_SRC}
)
TARGET_COMPILE_DEFINITIONS(integration_cpp_13 PRIVATE DAS_MOD_EXPORTS)
TARGET_LINK_LIBRARIES(integration_cpp_13 libDaScriptDyn Threads::Threads)
ADD_DEPENDENCIES(integration_cpp_13 libDaScriptDyn integration_cpp_13_dasAotStub)
SETUP_CPP11(integration_cpp_13)

#################################
# Tutorial 14 — Serialization
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_14
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/14_serialization.cpp
)

#################################
# Tutorial 15 — Custom Annotations
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_15
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/15_custom_annotations.cpp
)

#################################
# Tutorial 16 — Sandbox
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_16
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/16_sandbox.cpp
)

#################################
# Tutorial 17 — Coroutines
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_17
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/17_coroutines.cpp
)

#################################
# Tutorial 18 — Dynamic Scripts
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_18
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/18_dynamic_scripts.cpp
)

#################################
# Tutorial 19 — Class Adapters
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_19
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/19_class_adapters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/19_class_adapters.das
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/class_adapters_module.das
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/class_adapters_module.das.inc
)
target_include_directories(integration_cpp_19 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/builtin)
CMAKE_TEXT_XXD(integration_cpp_19 ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/class_adapters_module.das)
ADD_PROJECT_XXD_DEPENDS(integration_cpp_19)

#################################
# Tutorial 20 — Standalone Context
#################################
# Step 1: Generate standalone context files from the .das script.
SET(INTEGRATION_20_STANDALONE_CTX_GENERATED_SRC)
add_custom_target(integration_cpp_20_dasAotStubStandalone)
DAS_AOT_CTX("tutorials/integration/cpp/standalone_context.das"
            INTEGRATION_20_STANDALONE_CTX_GENERATED_SRC
            integration_cpp_20_dasAotStubStandalone daslang)

# Step 2: Build the tutorial executable with the generated files.
add_executable(integration_cpp_20
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/20_standalone_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/standalone_context.das
    ${INTEGRATION_20_STANDALONE_CTX_GENERATED_SRC}
)
TARGET_COMPILE_DEFINITIONS(integration_cpp_20 PRIVATE DAS_MOD_EXPORTS)
TARGET_LINK_LIBRARIES(integration_cpp_20 libDaScriptDyn Threads::Threads)
ADD_DEPENDENCIES(integration_cpp_20 libDaScriptDyn integration_cpp_20_dasAotStubStandalone)
SETUP_CPP11(integration_cpp_20)

#################################
# Tutorial 21 — Threading
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_21
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/21_threading.cpp
)

#################################
# Tutorial 22 — Namespace Integration
#################################
DAS_CPP_INTEGRATION_TUTORIAL(integration_cpp_22
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/22_namespace_integration.cpp
)

# Install C++ integration tutorial source files
file(GLOB CPP_INTEGRATION_TUTORIAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/*.das
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/*.inc
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/*.das_project
)
install(FILES ${CPP_INTEGRATION_TUTORIAL_SOURCES}
    DESTINATION ${DAS_INSTALL_TUTORIALSDIR}/integration/cpp
)

# Install standalone CMakeLists.txt for building tutorials against the installed SDK.
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/tutorials/integration/cpp/CMakeLists.standalone.cmake
    DESTINATION ${DAS_INSTALL_TUTORIALSDIR}/integration/cpp
    RENAME CMakeLists.txt
)
