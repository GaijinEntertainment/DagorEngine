add_executable(sq sq.cpp)
add_executable(squirrel::interpreter ALIAS sq)
set_target_properties(sq PROPERTIES EXPORT_NAME interpreter)
target_link_libraries(sq squirrel sqstdlib sqmodules)
target_include_directories(sq PUBLIC
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/sqmodules>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  )
