# Build daslang utilities as standalone executables using -exe mode.
# Each utility with a main.das is compiled via daslang -exe and the
# resulting binary is placed next to the daslang compiler binary
# (e.g. bin/Release/ on multi-config generators, bin/ on single-config).
# Requires LLVM for -exe compilation.

if(DAS_LLVM_DISABLED)
    return()
endif()

# JIT-linked util.exe pulls in libDaScriptDyn_runtime.so. When daslang is built
# with -fsanitize=*, the .so references sanitizer runtime helpers that the JIT
# link cmd (c++ without -fsanitize) can't satisfy. Forward the flag via
# --jit-linker-string so daslang's link cmd resolves the right runtime.
set(DAS_JIT_LINKER_STRING "")
if(DAS_USE_SANITIZER STREQUAL "address" OR DAS_USE_SANITIZER STREQUAL "asan")
    set(DAS_JIT_LINKER_STRING "--jit-linker-string=-fsanitize=address")
elseif(DAS_USE_SANITIZER STREQUAL "undefined" OR DAS_USE_SANITIZER STREQUAL "ubsan")
    set(DAS_JIT_LINKER_STRING "--jit-linker-string=-fsanitize=undefined")
elseif(DAS_USE_SANITIZER STREQUAL "thread" OR DAS_USE_SANITIZER STREQUAL "tsan")
    set(DAS_JIT_LINKER_STRING "--jit-linker-string=-fsanitize=thread")
endif()

set(DAS_UTILS
    aot
    dascov
    daspkg
    detect-dupe
    hygiene
    jit
    lint
)

if(NOT DAS_SQLITE_DISABLED)
    list(APPEND DAS_UTILS benchctl)
endif()

if(NOT DAS_HV_DISABLED)
    list(APPEND DAS_UTILS mcp)
endif()

# Collect daslib and LLVM daslib sources as dependencies. jit is a compiler-
# driver: edits anywhere under modules/dasLLVM/ (llvm_exe.das, llvm_jit_cli.das,
# llvm_jit_run.das, bindings/*.das) must trigger a jit.exe rebuild. Use a
# recursive glob so subdirs are covered.
file(GLOB DASLIB_SOURCES ${PROJECT_SOURCE_DIR}/daslib/*.das)
file(GLOB_RECURSE DASLLVM_DASLIB_SOURCES ${PROJECT_SOURCE_DIR}/modules/dasLLVM/*.das)

# daslang -exe appends `.exe` to the output path.
# On multi-config generators (Visual Studio), place next to daslang in
# bin/<Config>/. On single-config generators (Make/Ninja), place in bin/.
# Key on the GENERATOR, not the compiler: MSVC+Ninja (release CI) is
# single-config and its exes land in plain bin/.
get_property(_DAS_UTILS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_DAS_UTILS_MULTI_CONFIG)
    set(UTIL_BIN_DIR ${PROJECT_SOURCE_DIR}/bin/${CMAKE_CFG_INTDIR})
else()
    set(UTIL_BIN_DIR ${PROJECT_SOURCE_DIR}/bin)
endif()

foreach(util ${DAS_UTILS})
    set(UTIL_MAIN ${PROJECT_SOURCE_DIR}/utils/${util}/main.das)

    # jit and lint are compiler-drivers: their standalone exes recompile
    # arbitrary daslang at runtime (lint compiles each target to walk its AST),
    # so they must register every builtin native module at startup — otherwise
    # targets requiring math/fio/ast_core/... fail to resolve.
    set(_util_extra_args "")
    if(util STREQUAL "jit" OR util STREQUAL "lint")
        set(_util_extra_args --jit-register-all-modules)
    endif()

    add_custom_command(
        OUTPUT  ${UTIL_BIN_DIR}/${util}.exe
        COMMAND daslang -exe -output ${UTIL_BIN_DIR}/${util} ${UTIL_MAIN}
                $<$<OR:$<BOOL:${DAS_JIT_LINKER_STRING}>,$<BOOL:${_util_extra_args}>>:-->
                ${_util_extra_args}
                ${DAS_JIT_LINKER_STRING}
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
            $<$<BOOL:${DAS_JIT_LINKER_STRING}>:-->
            ${DAS_JIT_LINKER_STRING}
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

# Prewarm the JIT dll cache for the whole test tree, except the folders dastest
# also excludes under -jit (gc, ast_match, ast, no_aot — they use runtime quote/qmacro
# the JIT can't lower). Reuses the jit_exe target built by the foreach above (bin/jit.exe, built
# with --jit-register-all-modules). Run explicitly:
#   ninja jit_cache_all_tests
add_custom_target(jit_cache_all_tests
    COMMAND ${UTIL_BIN_DIR}/jit.exe ${PROJECT_SOURCE_DIR}/tests --parallel 0 $<IF:$<CONFIG:Debug>,--jit-opt-level=0,--jit-opt-level=3> --exclude gc --exclude dasHV --exclude dasPUGIXML --exclude dasSQLITE --exclude stbimage --exclude live_host --exclude audio --exclude strudel --exclude ast_match --exclude ast --exclude no_aot
    DEPENDS jit_exe
    WORKING_DIRECTORY ${UTIL_BIN_DIR}
    COMMENT "JIT-compiling tests/ into the .jitted_scripts cache"
)

SET(DAS_UTILS_TO_TEST
    dascov
    mcp
    lint
    detect-dupe
    find-dupe/tests
    hygiene
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
# Same generator-keyed split as UTIL_BIN_DIR above — under MSVC+Ninja the exes
# are in plain bin/, and bin/${CMAKE_INSTALL_CONFIG_NAME} would miss them all.
if(_DAS_UTILS_MULTI_CONFIG)
    set(_UTIL_INSTALL_PATTERN "${PROJECT_SOURCE_DIR}/bin/\${CMAKE_INSTALL_CONFIG_NAME}")
else()
    set(_UTIL_INSTALL_PATTERN "${PROJECT_SOURCE_DIR}/bin")
endif()
# `daslang -exe` writes its output via fopen("wb") which gives 0644 on Linux/
# macOS — no executable bit. file(INSTALL) defaults to preserving source perms,
# so without explicit FILE_PERMISSIONS the installed copy is non-executable
# and users must `chmod +x` before running. v0.6.2-RC3 Linux bundle shipped
# this way (all 8 daslang -exe outputs were `-rw-r--r--`).
foreach(util ${DAS_UTILS})
    install(CODE "
        set(_util_exe \"${_UTIL_INSTALL_PATTERN}/${util}.exe\")
        if(EXISTS \"\${_util_exe}\")
            file(INSTALL \"\${_util_exe}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${DAS_INSTALL_BINDIR}\"
                FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
        endif()
    ")
endforeach()
install(CODE "
    set(_util_exe \"${_UTIL_INSTALL_PATTERN}/dastest.exe\")
    if(EXISTS \"\${_util_exe}\")
        file(INSTALL \"\${_util_exe}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/${DAS_INSTALL_BINDIR}\"
            FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
")
