set(COMPILER_SRC
    ast.cpp
    compilationcontext.cpp
    parser.cpp
    codegen.cpp
    constgen.cpp
    sqio.cpp
    compiler.cpp
    sqfuncstate.cpp
    sqdump.cpp
    optimizer.cpp
    lexer.cpp
    sqtypeparser.cpp
    typeinference.cpp
    optimizations/closureHoisting.cpp
    static_analyzer/analyzer.cpp
    static_analyzer/checker_visitor.cpp
    static_analyzer/config.cpp
    static_analyzer/function_ret_type_eval.cpp
    static_analyzer/global_state.cpp
    static_analyzer/name_shadowing_checker.cpp
    static_analyzer/naming.cpp
    static_analyzer/value_ref.cpp
    static_analyzer/var_scope.cpp
    static_analyzer/external_value.cpp
)

add_library(quirrel-compiler STATIC ${COMPILER_SRC})
add_library(quirrel::compiler ALIAS quirrel-compiler)

# The compiler needs exceptions for error handling (CompilerError)
target_compile_options(quirrel-compiler PRIVATE
  "$<$<CXX_COMPILER_ID:GNU,Clang>:-fexceptions>"
  "$<$<CXX_COMPILER_ID:MSVC>:/EHsc>"
)

target_include_directories(quirrel-compiler PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/internal>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/helpers>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/squirrel/compiler>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/squirrel>"
  )
