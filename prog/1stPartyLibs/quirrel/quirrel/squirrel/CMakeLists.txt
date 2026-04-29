set(SQUIRREL_SRC sqapi.cpp
                 sqbaselib.cpp
                 sqclass.cpp
                 sqdebug.cpp
                 sqmem.cpp
                 sqobject.cpp
                 sqstate.cpp
                 sqtable.cpp
                 sqvm.cpp
                 sqdedupshrinker.cpp
                 sqstringlib.cpp
                 sqext.cpp)

if (ENABLE_VAR_TRACE)
  list(APPEND SQUIRREL_SRC vartrace.cpp)
  add_definitions(-DSQ_VAR_TRACE_ENABLED=1)
else ()
  list(APPEND SQUIRREL_SRC vartracestub.cpp)
  add_definitions(-DSQ_VAR_TRACE_ENABLED=0)
endif()


add_library(squirrel STATIC ${SQUIRREL_SRC})
add_library(squirrel::squirrel ALIAS squirrel)
target_include_directories(squirrel PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/internal>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/squirrel>"
  )
