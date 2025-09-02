set(SQSTDLIB_SRC sqstdaux.cpp
                 sqstdblob.cpp
                 sqstddebug.cpp
                 sqstdio.cpp
                 sqstdmath.cpp
                 sqstdrex.cpp
                 sqstdstream.cpp
                 sqstdstring.cpp
                 sqstddatetime.cpp
                 sqstdserialization.cpp
                 sqstdsystem.cpp)


add_library(sqstdlib STATIC ${SQSTDLIB_SRC})
add_library(squirrel::sqstdlib ALIAS sqstdlib)
set_property(TARGET sqstdlib PROPERTY EXPORT_NAME sqstdlib)
target_include_directories(sqstdlib PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/internal>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  )
