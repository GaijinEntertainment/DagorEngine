set(SQMODULES_SRC 
  path.cpp
  sqmodules.cpp
)


add_library(sqmodules STATIC ${SQMODULES_SRC})
add_library(squirrel::sqmodules ALIAS sqmodules)
set_property(TARGET sqmodules PROPERTY EXPORT_NAME sqmodules)
target_link_libraries(sqmodules sqstdlib squirrel)
target_include_directories(sqmodules PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  )
