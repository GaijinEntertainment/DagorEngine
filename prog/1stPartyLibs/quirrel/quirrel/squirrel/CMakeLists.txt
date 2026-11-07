set(SQUIRREL_SRC sqapi.cpp
                 sqasync.cpp
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
else ()
  list(APPEND SQUIRREL_SRC vartracestub.cpp)
endif()


add_library(squirrel STATIC ${SQUIRREL_SRC})
add_library(squirrel::squirrel ALIAS squirrel)
target_include_directories(squirrel PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  PRIVATE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/internal>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/squirrel>"
  )
