# CI lane targets: verify formatting / regenerated-bindings freshness. Both run
# daslang interpreted (no LLVM), so define them before the DAS_LLVM_DISABLED
# early-return below.
add_custom_target(check_format
    COMMAND $<TARGET_FILE:daslang> ${PROJECT_SOURCE_DIR}/utils/das-fmt/dasfmt.das -- --path ${PROJECT_SOURCE_DIR}/ --verify
    DEPENDS daslang
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Verifying .das formatting (das-fmt --verify)"
)

add_custom_target(check_dasgen
    COMMAND $<TARGET_FILE:daslang> ${PROJECT_SOURCE_DIR}/utils/dasgen/gen_bind.das
    COMMAND git diff --exit-code -- ${PROJECT_SOURCE_DIR}/include/daScript/builtin/
    DEPENDS daslang
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Checking generated builtin .inc files are up to date (dasgen)"
)

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

# Prewarm the JIT dll cache for the whole test tree, except module-gated folders and gc
# (mirrors tests/.das_test: gc stays interp-only under -jit — heap_collect can't see
# jitted-frame locals; quote/qmacro folders prewarm fine now that jit_enabled triggers
# daslib/quote lowering). Reuses the jit_exe target built by the foreach above
# (bin/jit.exe, built with --jit-register-all-modules). Run explicitly:
#   ninja jit_cache_all_tests
add_custom_target(jit_cache_all_tests
    COMMAND ${UTIL_BIN_DIR}/jit.exe ${PROJECT_SOURCE_DIR}/tests --parallel 0 $<IF:$<CONFIG:Debug>,--jit-opt-level=0,--jit-opt-level=3> --exclude gc --exclude dasHV --exclude dasPUGIXML --exclude dasSQLITE --exclude sql_conformance --exclude stbimage --exclude dasGLTF --exclude live_host --exclude audio --exclude strudel --exclude strudel_device --exclude minfft
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
    ast-fuzz
)

FOREACH(_das ${DAS_UTILS_TO_TEST})
    LIST(APPEND _run_utils_tests
        COMMAND ${CMAKE_COMMAND} -E echo "Running tests ${_das}"
        COMMAND daslang ${PROJECT_SOURCE_DIR}/dastest/dastest.das -- --test ${PROJECT_SOURCE_DIR}/utils/${_das}
        DEPENDS gen1_to_gen2 tree_sitter_daslang
    )
ENDFOREACH()

# gen1_to_gen2 (utils/dasFormatter, the gen1->gen2 syntax converter) carries its own
# --tests self-check suite. It's a C++ target (not a daslang -exe util), so invoke
# the binary directly rather than through dastest. Previously uncovered by CI, which
# let the make-literal verify-compile tests rot unnoticed. (issue #3094)
LIST(APPEND _run_utils_tests
    COMMAND ${CMAKE_COMMAND} -E echo "Running tests gen1_to_gen2 dasFormatter --tests"
    COMMAND $<TARGET_FILE:gen1_to_gen2> --tests
    DEPENDS gen1_to_gen2
)
ADD_CUSTOM_TARGET(run_utils_tests
    ${_run_utils_tests}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running utils tests"
)
