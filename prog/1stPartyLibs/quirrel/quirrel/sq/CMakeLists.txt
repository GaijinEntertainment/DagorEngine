add_executable(sq sq.cpp sq_test_natives.cpp)

target_link_libraries(sq squirrel sqstdlib sqmodules quirrel-compiler)
target_include_directories(sq PRIVATE
  # include/ and sqmodules/ are transitive from linked libraries
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/sqrat/include>"
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/squirrel>" # Needed for sqtypeparser.h
  )
