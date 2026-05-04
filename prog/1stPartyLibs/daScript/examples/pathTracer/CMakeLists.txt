### 1. setup custom AOT utility
SET(PATHTRACER_DASAOT_MAIN_SRC
    ${DAS_DASCRIPT_MAIN_SRC}
)

### 2. setup aot target for the path tracer
add_custom_target(pathtracer_dasAotStub)
SET(PATHTRACER_AOT_GENERATED_SRC)
DAS_AOT("examples/pathTracer/path_tracer.das" PATHTRACER_AOT_GENERATED_SRC pathtracer_dasAotStub daslang)

### setup main path tracer executable
SET(PATHTRACER_AOT_SRC
${CMAKE_CURRENT_SOURCE_DIR}/examples/pathTracer/path_tracer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/examples/pathTracer/path_tracer.das
${CMAKE_CURRENT_SOURCE_DIR}/examples/pathTracer/toy_path_tracer_profile.das
)
add_executable(path_tracer ${PATHTRACER_AOT_SRC} ${PATHTRACER_AOT_GENERATED_SRC})
target_include_directories(path_tracer PUBLIC ${PROJECT_SOURCE_DIR}/examples/pathTracer)
TARGET_LINK_LIBRARIES(path_tracer libDaScriptAot ${DAS_MODULES_LIBS})
ADD_DEPENDENCIES(path_tracer libDaScriptAot pathtracer_dasAotStub)
SETUP_CPP11(path_tracer)

install(FILES
    ${PROJECT_SOURCE_DIR}/examples/pathTracer/path_tracer.das
    ${PROJECT_SOURCE_DIR}/examples/pathTracer/toy_path_tracer.das
    ${PROJECT_SOURCE_DIR}/examples/pathTracer/toy_pathtracer_opengl_basic.das
    ${PROJECT_SOURCE_DIR}/examples/pathTracer/toy_path_tracer_opengl.das
    ${PROJECT_SOURCE_DIR}/examples/pathTracer/toy_path_tracer_opengl_hdr.das
    ${PROJECT_SOURCE_DIR}/examples/pathTracer/toy_path_tracer_profile.das
    DESTINATION ${DAS_INSTALL_EXAMPLESDIR}/pathTracer)
