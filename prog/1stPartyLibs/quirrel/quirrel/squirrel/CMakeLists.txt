set(SQUIRREL_SRC sqapi.cpp
                 sqbaselib.cpp
                 sqast.cpp
                 sqcompilationcontext.cpp
                 sqbinaryast.cpp
                 sqastparser.cpp
                 sqastcodegen.cpp
                 sqclass.cpp
                 sqcompiler.cpp
                 sqdebug.cpp
                 sqdirect.cpp
                 sqfuncstate.cpp
                 sqoptimizer.cpp
                 sqstringlib.cpp
                 sqlexer.cpp
                 sqmem.cpp
                 sqobject.cpp
                 sqstate.cpp
                 sqdedupshrinker.cpp
                 sqtable.cpp
                 sqvm.cpp
                 optimizations/closureHoisting.cpp
                 static_analyzer/analyzer.cpp)

if (ENABLE_VAR_TRACE)
  list(APPEND SQUIRREL_SRC vartrace.cpp)
  add_definitions(-DSQ_VAR_TRACE_ENABLED=1)
else ()
  list(APPEND SQUIRREL_SRC vartracestub.cpp)
  add_definitions(-DSQ_VAR_TRACE_ENABLED=0)
endif()


add_library(squirrel STATIC ${SQUIRREL_SRC})
add_library(squirrel::squirrel ALIAS squirrel)
set_property(TARGET squirrel PROPERTY EXPORT_NAME squirrel)
target_include_directories(squirrel PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/internal>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/helpers>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  )
