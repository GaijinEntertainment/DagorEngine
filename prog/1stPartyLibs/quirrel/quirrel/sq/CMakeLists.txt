if(NOT DISABLE_DYNAMIC)
  add_executable(sq sq.cpp)
  add_executable(squirrel::interpreter ALIAS sq)
  set_target_properties(sq PROPERTIES EXPORT_NAME interpreter)
  target_link_libraries(sq squirrel sqstdlib sqmodules)
  if(NOT SQ_DISABLE_INSTALLER)
    install(TARGETS sq EXPORT squirrel RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime)
  endif()
  target_include_directories(sq PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/sqmodules>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
endif()

if(NOT DISABLE_STATIC)
  add_executable(sq_static sq.cpp)
  add_executable(squirrel::interpreter_static ALIAS sq_static)
  set_target_properties(sq_static PROPERTIES EXPORT_NAME interpreter_static)
  target_link_libraries(sq_static squirrel_static sqstdlib_static sqmodules_static)
  if(NOT SQ_DISABLE_INSTALLER)
    install(TARGETS sq_static EXPORT squirrel RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime)
  endif()
  target_include_directories(sq_static PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/sqmodules>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
endif()

if(LONG_OUTPUT_NAMES)
  if(NOT DISABLE_DYNAMIC)
    set_target_properties(sq PROPERTIES OUTPUT_NAME squirrel3)
  endif()

  if(NOT DISABLE_STATIC)
    set_target_properties(sq_static PROPERTIES OUTPUT_NAME squirrel3_static)
  endif()
endif()
