set(SQMODULES_SRC 
  path.cpp
  sqmodules.cpp
)


if(NOT DISABLE_DYNAMIC)
  add_library(sqmodules SHARED ${SQMODULES_SRC})
  add_library(squirrel::sqmodules ALIAS sqmodules)
  set_property(TARGET sqmodules PROPERTY EXPORT_NAME sqmodules)
  target_link_libraries(sqmodules sqstdlib squirrel)
  if(NOT SQ_DISABLE_INSTALLER)
    install(TARGETS sqmodules EXPORT squirrel
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Libraries
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Libraries NAMELINK_SKIP
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Libraries
      )
    install(TARGETS sqmodules EXPORT squirrel
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development NAMELINK_ONLY
      )
  endif()
  target_include_directories(sqmodules PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
endif()

if(NOT DISABLE_STATIC)
  add_library(sqmodules_static STATIC ${SQMODULES_SRC})
  add_library(squirrel::sqmodules_static ALIAS sqmodules_static)
  set_property(TARGET sqmodules_static PROPERTY EXPORT_NAME sqmodules_static)
  target_link_libraries(sqmodules_static sqstdlib_static squirrel_static)
  if(NOT SQ_DISABLE_INSTALLER)
    install(TARGETS sqmodules_static EXPORT squirrel ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development)
  endif()
  target_include_directories(sqmodules_static PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    )
endif()

if(LONG_OUTPUT_NAMES)
  if(NOT DISABLE_DYNAMIC)
    set_target_properties(sqmodules PROPERTIES OUTPUT_NAME sqmodules)
  endif()

  if(NOT DISABLE_STATIC)
    set_target_properties(sqmodules_static PROPERTIES OUTPUT_NAME sqmodules_static)
  endif()
endif()
