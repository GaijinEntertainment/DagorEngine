# Build daslang utilities as standalone executables using -exe mode.
# Each utility with a main.das is compiled via daslang -exe and the
# resulting binary is placed next to the daslang compiler binary
# (e.g. bin/Release/ on multi-config generators, bin/ on single-config).
# Requires LLVM for -exe compilation.

if(DAS_LLVM_DISABLED)
    return()
endif()

set(DAS_UTILS
    aot
    dascov
    daspkg
)

if(NOT DAS_SQLITE_DISABLED)
    list(APPEND DAS_UTILS benchctl)
endif()

if(NOT DAS_HV_DISABLED)
    list(APPEND DAS_UTILS mcp)
endif()

# Collect daslib and LLVM daslib sources as dependencies.
file(GLOB DASLIB_SOURCES ${PROJECT_SOURCE_DIR}/daslib/*.das)
file(GLOB DASLLVM_DASLIB_SOURCES ${PROJECT_SOURCE_DIR}/modules/dasLLVM/daslib/*.das)

# daslang -exe appends `.exe` to the output path.
# On multi-config generators (MSVC), place next to daslang in bin/<Config>/.
# On single-config generators (Make/Ninja), place in bin/.
if(MSVC)
    set(UTIL_BIN_DIR ${PROJECT_SOURCE_DIR}/bin/${CMAKE_CFG_INTDIR})
else()
    set(UTIL_BIN_DIR ${PROJECT_SOURCE_DIR}/bin)
endif()

foreach(util ${DAS_UTILS})
    set(UTIL_MAIN ${PROJECT_SOURCE_DIR}/utils/${util}/main.das)

    add_custom_command(
        OUTPUT  ${UTIL_BIN_DIR}/${util}.exe
        COMMAND daslang -exe -output ${UTIL_BIN_DIR}/${util} ${UTIL_MAIN}
        DEPENDS ${UTIL_MAIN} daslang ${DASLIB_SOURCES} ${DASLLVM_DASLIB_SOURCES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Building ${util} executable (daslang -exe)"
    )

    add_custom_target(${util}_exe DEPENDS ${UTIL_BIN_DIR}/${util}.exe)
endforeach()

SET(TEST_MAIN ${PROJECT_SOURCE_DIR}/dastest/dastest.das)

add_custom_command(
    OUTPUT  ${UTIL_BIN_DIR}/dastest.exe
    COMMAND daslang -exe -output ${UTIL_BIN_DIR}/dastest ${TEST_MAIN}
    DEPENDS ${TEST_MAIN} daslang ${DASLIB_SOURCES} ${DASLLVM_DASLIB_SOURCES}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Building dastest executable (daslang -exe)"
)
add_custom_target(dastest_exe DEPENDS ${UTIL_BIN_DIR}/dastest.exe)


add_custom_target(all_utils_exe ALL)
foreach(util ${DAS_UTILS})
    add_dependencies(all_utils_exe ${util}_exe)
endforeach()
add_dependencies(all_utils_exe dastest_exe)

SET(DAS_UTILS_TO_TEST
    dascov
    mcp
    perf_lint
)

FOREACH(_das ${DAS_UTILS_TO_TEST})
    LIST(APPEND _run_utils_tests
        COMMAND ${CMAKE_COMMAND} -E echo "Running tests ${_das}"
        COMMAND daslang ${PROJECT_SOURCE_DIR}/dastest/dastest.das -- --test ${PROJECT_SOURCE_DIR}/utils/${_das}
        DEPENDS das-fmt tree_sitter_daslang
    )
ENDFOREACH()
ADD_CUSTOM_TARGET(run_utils_tests
    ${_run_utils_tests}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running utils tests"
)

# Install utility executables (OPTIONAL — only present when -exe targets are built).
# UTIL_BIN_DIR uses CMAKE_CFG_INTDIR which doesn't resolve at install time on
# multi-config generators. Use install(CODE) with CMAKE_INSTALL_CONFIG_NAME.
if(MSVC)
    set(_UTIL_INSTALL_PATTERN "${PROJECT_SOURCE_DIR}/bin/\${CMAKE_INSTALL_CONFIG_NAME}")
else()
    set(_UTIL_INSTALL_PATTERN "${PROJECT_SOURCE_DIR}/bin")
endif()
foreach(util ${DAS_UTILS})
    install(CODE "
        set(_util_exe \"${_UTIL_INSTALL_PATTERN}/${util}.exe\")
        if(EXISTS \"\${_util_exe}\")
            file(INSTALL \"\${_util_exe}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${DAS_INSTALL_BINDIR}\")
        endif()
    ")
endforeach()
install(CODE "
    set(_util_exe \"${_UTIL_INSTALL_PATTERN}/dastest.exe\")
    if(EXISTS \"\${_util_exe}\")
        file(INSTALL \"\${_util_exe}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${DAS_INSTALL_BINDIR}\")
    endif()
")
