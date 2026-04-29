set(SQSTDLIB_SRC sqstdaux.cpp
                 sqstdblob.cpp
                 sqstddebug.cpp
                 sqstdio.cpp
                 sqstdhash.cpp
                 sqstdmath.cpp
                 sqstdrex.cpp
                 sqstdstream.cpp
                 sqstdstring.cpp
                 sqstddatetime.cpp
                 sqstdserialization.cpp
                 sqstdsystem.cpp)


add_library(sqstdlib STATIC ${SQSTDLIB_SRC})
add_library(squirrel::sqstdlib ALIAS sqstdlib)

target_include_directories(sqstdlib PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/internal>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/squirrel>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  )
