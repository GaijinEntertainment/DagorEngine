
cmake_minimum_required(VERSION 2.8)

project(HLSLcc)

option(HLSLCC_LIBRARY_SHARED "Build shared library instead of static." OFF)

set(HLSLCC_SRC
		src/ControlFlowGraph.cpp
		src/ControlFlowGraphUtils.cpp
		src/DataTypeAnalysis.cpp
		src/Declaration.cpp
		src/decode.cpp
		src/HLSLcc.cpp
		src/HLSLccToolkit.cpp
		src/HLSLCrossCompilerContext.cpp
		src/Instruction.cpp
		src/LoopTransform.cpp
		src/Operand.cpp
		src/reflect.cpp
		src/Shader.cpp
		src/ShaderInfo.cpp
		src/toGLSL.cpp
		src/toGLSLDeclaration.cpp
		src/toGLSLInstruction.cpp
		src/toGLSLOperand.cpp
		src/toMetal.cpp
		src/toMetalDeclaration.cpp
		src/toMetalInstruction.cpp
		src/toMetalOperand.cpp
		src/UseDefineChains.cpp
		src/cbstring/bsafe.c
		src/cbstring/bstraux.c
		src/cbstring/bstrlib.c)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/internal_includes"
		"${CMAKE_CURRENT_SOURCE_DIR}/src/cbstring"
		"${CMAKE_CURRENT_SOURCE_DIR}/src")

if(HLSLCC_LIBRARY_SHARED)
	add_library(hlslcc SHARED ${HLSLCC_SRC})
else(HLSLCC_LIBRARY_SHARED)
	add_library(hlslcc STATIC ${HLSLCC_SRC})
endif(HLSLCC_LIBRARY_SHARED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
set_property(TARGET hlslcc PROPERTY CXX_STANDARD 11)
add_definitions(-D_CRT_SECURE_NO_WARNINGS=1)
add_definitions(-D_SCL_SECURE_NO_WARNINGS=1)

enable_testing()

add_executable(tests EXCLUDE_FROM_ALL ${HLSLCC_SRC} test.cpp)

add_test(test1 tests COMMAND tests)
