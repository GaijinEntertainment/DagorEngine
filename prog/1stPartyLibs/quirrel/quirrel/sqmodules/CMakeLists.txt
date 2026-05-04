set(SQMODULES_SRC 
  sqmodules.cpp
  helpers.cpp
  path.cpp
  deffileaccess.cpp
)


add_library(sqmodules STATIC ${SQMODULES_SRC})
add_library(squirrel::sqmodules ALIAS sqmodules)

target_link_libraries(sqmodules sqstdlib squirrel)
target_include_directories(sqmodules PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/sqmodules>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/sqrat/include>"
  )
