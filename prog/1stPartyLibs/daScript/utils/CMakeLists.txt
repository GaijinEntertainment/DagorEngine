# CI lane targets: verify formatting / regenerated-bindings freshness. Both run
# daslang interpreted (no LLVM), so define them before the DAS_LLVM_DISABLED
# early-return below.
file(GLOB_RECURSE _das_format_inputs
    ${PROJECT_SOURCE_DIR}/daslib/*.das ${PROJECT_SOURCE_DIR}/dastest/*.das
    ${PROJECT_SOURCE_DIR}/tests/*.das ${PROJECT_SOURCE_DIR}/utils/*.das
    ${PROJECT_SOURCE_DIR}/examples/*.das ${PROJECT_SOURCE_DIR}/tutorials/*.das
    ${PROJECT_SOURCE_DIR}/modules/*.das ${PROJECT_SOURCE_DIR}/benchmarks/*.das)
add_das_target(check_format
    COMMAND $<TARGET_FILE:daslang> ${PROJECT_SOURCE_DIR}/utils/das-fmt/dasfmt.das -- --path ${PROJECT_SOURCE_DIR}/ --verify --exclude-mask ${CMAKE_BINARY_DIR}
    DEPENDS daslang ${_das_format_inputs}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Verifying .das formatting (das-fmt --verify)"
)

add_custom_target(check_dasgen
    COMMAND $<TARGET_FILE:daslang> ${PROJECT_SOURCE_DIR}/utils/internal/dasgen/gen_bind.das
    COMMAND git diff --exit-code -- ${PROJECT_SOURCE_DIR}/include/daScript/builtin/
    DEPENDS daslang
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Checking generated builtin .inc files are up to date (dasgen)"
)

# The lint gate's -P script; configure emits it into the build dir.
set(_das_changed_files_fn [==[
function(das_ci_changed_files out base_ref)
    execute_process(
        COMMAND git diff --name-only --diff-filter=AM "origin/${base_ref}...HEAD" -- ${ARGN}
        WORKING_DIRECTORY "${DAS_ROOT}"
        OUTPUT_VARIABLE _files
        RESULT_VARIABLE _rc
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT _rc EQUAL 0)
        message(FATAL_ERROR "git diff against origin/${base_ref} failed (${_rc})")
    endif()
    string(REPLACE "\n" ";" _files "${_files}")
    set(${out} "${_files}" PARENT_SCOPE)
endfunction()
]==])

set(DAS_LINT_CHANGED_SCRIPT ${CMAKE_BINARY_DIR}/das_lint_changed.cmake)
file(WRITE ${DAS_LINT_CHANGED_SCRIPT} "${_das_changed_files_fn}" [==[
if(NOT BASE_REF)
    set(BASE_REF "$ENV{DAS_CI_BASE_REF}")
endif()
if(NOT BASE_REF)
    set(BASE_REF master)
endif()
das_ci_changed_files(_changed "${BASE_REF}" "*.das" "*.md")
if(NOT _changed)
    message(STATUS "no .das or .md files changed; skipping lint")
    return()
endif()
message(STATUS "linting: ${_changed}")
execute_process(
    COMMAND "${DASLANG}" ./utils/lint/main.das -- ${_changed} --quiet --disable LINT019
    WORKING_DIRECTORY "${DAS_ROOT}"
    RESULT_VARIABLE _rc)
if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "lint reported findings (${_rc})")
endif()
]==])

add_das_target(check_lint_changed
    COMMAND ${CMAKE_COMMAND} -DDAS_ROOT=${PROJECT_SOURCE_DIR} -DDASLANG=$<TARGET_FILE:daslang>
            -P ${DAS_LINT_CHANGED_SCRIPT}
    DEPENDS daslang
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Linting the .das and .md files changed against origin/DAS_CI_BASE_REF"
)
add_custom_target(check_lint_tree
    COMMAND $<TARGET_FILE:daslang> ./utils/lint/main.das -- daslib utils modules tutorials examples tests src
            -q --silent -j 1
    DEPENDS daslang
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Linting the whole tree in one process"
)

set(_dupe_paths -p daslib -p utils -p src)
file(GLOB _dupe_entries LIST_DIRECTORIES true RELATIVE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/modules/*/*)
foreach(_e IN LISTS _dupe_entries)
    if(_e MATCHES "\\.das$" OR (IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${_e} AND NOT _e MATCHES "/(tutorials|examples)$"))
        list(APPEND _dupe_paths -p ${_e})
    endif()
endforeach()
add_custom_target(check_duplicates
    COMMAND $<TARGET_FILE:daslang> utils/detect-dupe/main.das -- ${_dupe_paths} --no-fuzzy --min-tokens 64
    DEPENDS daslang
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Reporting exact duplicate function bodies across daslib, utils, src and modules")

# watchdog: the supervisor as a standalone context on the full runtime - libDaScript, dasHV,
# dasStdDlg (the tray icon) and dasStbImage (the icon file's PNG decoder) as static archives, no
# compiler, no shared module, no lock on the files it deploys (the shape of
# examples/standalone/06_full_runtime). Lands in bin/ and installs beside daslang. It needs no
# LLVM, so it sits above the -exe utilities' early return; the module targets exist only when
# their modules are enabled and included.
if(TARGET libDasModuleHV AND TARGET libDasModuleStdDlg AND TARGET libDasModuleStbImage)
    set(WATCHDOG_GEN "${CMAKE_CURRENT_BINARY_DIR}/_watchdog_ctx_generated")
    file(MAKE_DIRECTORY "${WATCHDOG_GEN}")
    file(GLOB WATCHDOG_DAS_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/utils/watchdog/*.das")
    add_custom_command(
        OUTPUT "${WATCHDOG_GEN}/main.das.cpp" "${WATCHDOG_GEN}/main.das.h"
        COMMAND $<TARGET_FILE:daslang>
            "${PROJECT_SOURCE_DIR}/utils/aot/main.das"
            -- -ctx "${PROJECT_SOURCE_DIR}/utils/watchdog/main.das"
            "${WATCHDOG_GEN}/"
        DEPENDS daslang
            ${WATCHDOG_DAS_SOURCES}
            "${PROJECT_SOURCE_DIR}/utils/aot/main.das"
            "${PROJECT_SOURCE_DIR}/daslib/aot_standalone.das"
            "${PROJECT_SOURCE_DIR}/daslib/aot_cpp.das"
        "${PROJECT_SOURCE_DIR}/daslib/c_api_header.das"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Standalone AOT (full runtime): watchdog"
        VERBATIM
    )
    add_executable(watchdog
        "${PROJECT_SOURCE_DIR}/utils/watchdog/main.cpp" "${WATCHDOG_GEN}/main.das.cpp")
    target_include_directories(watchdog PRIVATE
        "${WATCHDOG_GEN}"
        ${NEED_MODULES_PATH}
        "${PROJECT_SOURCE_DIR}/modules/dasHV/hv/$<CONFIG>/include")
    target_link_libraries(watchdog PRIVATE libDaScript ${SRC_LIBRARIES} libDasModuleHV libDasModuleStdDlg libDasModuleStbImage)
    SETUP_CPP11(watchdog)
    SETUP_BIN_RPATH(watchdog)
    set_target_properties(watchdog PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
        FOLDER "utils")
    if(NOT MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release")
        add_custom_command(TARGET watchdog POST_BUILD
            COMMAND ${CMAKE_STRIP} $<TARGET_FILE:watchdog>
            COMMENT "Stripping watchdog")
    endif()
    # A multi-config generator lands the exe in bin/<Config>/; the checked-in LSP plugin manifest
    # and every other hand-written spelling name bin/watchdog, so the built exe is copied there too
    get_property(_DAS_WATCHDOG_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(_DAS_WATCHDOG_MULTI_CONFIG)
        add_custom_command(TARGET watchdog POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:watchdog> ${PROJECT_SOURCE_DIR}/bin/$<TARGET_FILE_NAME:watchdog>
            COMMENT "Copying watchdog to bin/")
    endif()
    INSTALL(TARGETS watchdog RUNTIME DESTINATION ${DAS_INSTALL_BINDIR})
    add_test(NAME watchdog_smoke
        COMMAND ${CMAKE_COMMAND}
            -DWATCHDOG=$<TARGET_FILE:watchdog>
            -DDASLANG=$<TARGET_FILE:daslang>
            -DDAS_ROOT=${PROJECT_SOURCE_DIR}
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/watchdog_smoke
            -P ${PROJECT_SOURCE_DIR}/utils/watchdog/smoke_test.cmake)
    set_tests_properties(watchdog_smoke PROPERTIES LABELS "small" TIMEOUT 120)
    add_dependencies(test-small watchdog)
endif()

# Build daslang utilities as standalone executables using -exe mode.
# Each utility's entry .das is compiled via daslang -exe and the
# resulting binary is placed next to the daslang compiler binary
# (e.g. bin/Release/ on multi-config generators, bin/ on single-config).
# Requires LLVM for -exe compilation.


set(_lane_util_bin ${UTIL_BIN_DIR})


das_lane_dastest_target(run_tests_daspkg COMMENT "Testing daspkg"
    FILES utils/daspkg/test_daspkg.das ARGS --timeout 600)
add_custom_target(run_daspkg_tests
    COMMAND $<TARGET_FILE:daslang> ${PROJECT_SOURCE_DIR}/dastest/dastest.das -- --color --failures-only
            --test ${PROJECT_SOURCE_DIR}/utils/daspkg/test_daspkg_git.das --timeout 1200
    COMMAND ${CMAKE_COMMAND} -DDASLANG=$<TARGET_FILE:daslang> -DROOT=${PROJECT_SOURCE_DIR}
            -DOUT=${CMAKE_CURRENT_BINARY_DIR} -P ${PROJECT_SOURCE_DIR}/utils/daspkg/index_sweep.cmake
    DEPENDS daslang
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Testing daspkg: the unit and fixture-repo suites, then every daspkg-index package at default-branch HEAD"
    USES_TERMINAL
)
add_dependencies(run_daspkg_tests run_tests_daspkg)

if(NOT DAS_SQLITE_DISABLED)
    set(BENCH_STAND_OUT ${CMAKE_BINARY_DIR}/bench-stand CACHE PATH "Where run_bench_stand writes runs/ and site/")
    set(BENCH_STAND_ARGS "" CACHE STRING "Extra benchctl run arguments, e.g. --filter core/math/ --repeat 1")
    # the jit lane runs `daslang -jit`; a linked test_llvm_aot host is picked up when a box built
    # one, but the night never builds it - its LLVM-AOT corpus batches same-named modules
    set(_bench_stand_deps daslang)
    foreach(_t test_aot_bench dasModuleSQLITE dasModulePUGIXML dasModuleAudio dasModuleMinfft
            dasModuleTerminal dasModuleUnitTest dasModuleLLVM)
        if(TARGET ${_t})
            list(APPEND _bench_stand_deps ${_t})
        endif()
    endforeach()
    add_custom_target(run_bench_stand
        COMMAND ${CMAKE_COMMAND} -DDASLANG=$<TARGET_FILE:daslang> -DROOT=${PROJECT_SOURCE_DIR}
                -DOUT=${BENCH_STAND_OUT} "-DARGS=${BENCH_STAND_ARGS}"
                -P ${PROJECT_SOURCE_DIR}/utils/internal/bench-stand/stand.cmake
        DEPENDS ${_bench_stand_deps}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Benchmarking the tree into ${BENCH_STAND_OUT}"
        USES_TERMINAL
    )
endif()

das_lane_dastest_target(run_tests_mcp_tools
    COMMENT "Testing the MCP tools"
    FILES utils/mcp/test_tools.das utils/mcp/test_crosstree_guard.das utils/mcp/test_setup.das
)

das_lane_dastest_target(run_tests_dap_bridge COMMENT "Testing the DAP MCP bridge"
    FILES utils/dap/test_dap_bridge.das ARGS --timeout 900)

das_lane_dastest_target(run_tests_review_md COMMENT "Testing review-md discovery, the make-pr gates and preflight"
    FILES utils/internal/review-md/test_scan.das utils/internal/review-md/test_gate.das
          utils/internal/review-md/test_walkers.das utils/internal/make-pr/test_gates.das
          utils/internal/preflight/tests)

das_lane_dastest_target(run_tests_dastest COMMENT "Testing dastest's own suite" FILES dastest/tests)

das_lane_dastest_target(run_tests_pr_babysit COMMENT "Testing the pr-babysit verdict core"
    FILES utils/internal/pr-babysit/test_watch_verdict.das)

set(_ci_dasweb_playground)
foreach(_s test_samples_store test_curated_import test_playground_config test_playground_server test_build_queue test_build_artifacts test_build_endpoints)
    list(APPEND _ci_dasweb_playground utils/internal/dasweb-playground/${_s}.das)
endforeach()
das_lane_dastest_target(run_tests_dasweb_playground COMMENT "Testing dasweb-playground" FILES ${_ci_dasweb_playground})

set(_ci_dasllama_ladder)
foreach(_s test_ladder_store test_ladder_config test_ladder_server)
    list(APPEND _ci_dasllama_ladder utils/internal/dasllama-ladder/${_s}.das)
endforeach()
das_lane_dastest_target(run_tests_dasllama_ladder COMMENT "Testing dasllama-ladder" FILES ${_ci_dasllama_ladder})

set(_ci_dasweb_buildd)
foreach(_s test_buildd_config test_buildd_core test_buildd_client test_roll_toolchain)
    list(APPEND _ci_dasweb_buildd utils/internal/dasweb-buildd/${_s}.das)
endforeach()
das_lane_dastest_target(run_tests_dasweb_buildd COMMENT "Testing dasweb-buildd" FILES ${_ci_dasweb_buildd})

add_das_target(run_tests_dasweb_verify
    COMMAND $<TARGET_FILE:daslang> ${PROJECT_SOURCE_DIR}/dastest/dastest.das -- --color --failures-only --test ${PROJECT_SOURCE_DIR}/utils/internal/dasweb-verify/test_verify_core.das
    COMMAND $<TARGET_FILE:daslang> ./utils/internal/dasweb-verify/main.das
    DEPENDS daslang
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Testing dasweb-verify and compiling the curated samples"
)

add_custom_target(check_review_gates
    COMMAND $<TARGET_FILE:daslang> ./utils/internal/review-md/all.das
    DEPENDS daslang
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running every REVIEW.das gate"
)

add_das_target(check_gated_das
    COMMAND $<TARGET_FILE:daslang> utils/internal/preflight/main.das -- --only ci-das
    DEPENDS daslang
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Compiling the CI-only das surface (ci_only_das.txt)"
)

add_das_target(check_ast_verify_changed
    COMMAND $<TARGET_FILE:daslang> ./utils/ast-verify/main.das -- --changed
    DEPENDS daslang
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running --ast-verify-batch over the .das files changed against origin/DAS_CI_BASE_REF"
)

if(DAS_LLVM_DISABLED)
    return()
endif()

# Entries are paths relative to utils/; the exe/target name is the basename.
# mcp/lsp never build as exe: their development runs interpreted under a supervisor
# (the watchdog's --stdio and --lsp fronts), so an exe form would never be dogfooded.
set(DAS_UTILS
    aot
    dascov
    daspkg
    das-fmt
    detect-dupe
    lint
    internal/hygiene
    internal/jit
)

if(NOT DAS_SQLITE_DISABLED)
    list(APPEND DAS_UTILS benchctl)   # same gate on DAS_UTILS_SHIPPED_EXES below
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

foreach(util_path ${DAS_UTILS})
    get_filename_component(util ${util_path} NAME)
    # dasfmt.das is das-fmt's published entry point — external CIs and the
    # shipped pre-commit hook invoke it by name, so it does not rename to main.das
    if(util STREQUAL "das-fmt")
        set(UTIL_MAIN ${PROJECT_SOURCE_DIR}/utils/${util_path}/dasfmt.das)
    else()
        set(UTIL_MAIN ${PROJECT_SOURCE_DIR}/utils/${util_path}/main.das)
    endif()

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
file(GLOB DASTEST_SOURCES ${PROJECT_SOURCE_DIR}/dastest/*.das)

add_custom_command(
    OUTPUT  ${UTIL_BIN_DIR}/dastest.exe
    COMMAND daslang -exe -output ${UTIL_BIN_DIR}/dastest ${TEST_MAIN}
            $<$<BOOL:${DAS_JIT_LINKER_STRING}>:-->
            ${DAS_JIT_LINKER_STRING}
    DEPENDS ${TEST_MAIN} ${DASTEST_SOURCES} daslang ${DASLIB_SOURCES} ${DASLLVM_DASLIB_SOURCES}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Building dastest executable (daslang -exe)"
)
add_custom_target(dastest_exe DEPENDS ${UTIL_BIN_DIR}/dastest.exe)


add_custom_target(all_utils_exe ALL)
foreach(util_path ${DAS_UTILS})
    get_filename_component(util ${util_path} NAME)
    add_dependencies(all_utils_exe ${util}_exe)
endforeach()
add_dependencies(all_utils_exe dastest_exe)

# Single-config generators only: UTIL_BIN_DIR embeds CMAKE_CFG_INTDIR on
# multi-config, which install() cannot resolve — and release bundles are cut on
# single-config (Ninja) lanes. Each shipped tool's decision is recorded in that
# tool's README.md, where utils/REVIEW.das reads it.
if(NOT _DAS_UTILS_MULTI_CONFIG)
    set(DAS_UTILS_SHIPPED_EXES lint daspkg dascov detect-dupe das-fmt dastest)
    if(NOT DAS_SQLITE_DISABLED)
        list(APPEND DAS_UTILS_SHIPPED_EXES benchctl)   # same gate on DAS_UTILS above
    endif()
    foreach(util ${DAS_UTILS_SHIPPED_EXES})
        install(PROGRAMS ${UTIL_BIN_DIR}/${util}.exe DESTINATION ${DAS_INSTALL_BINDIR})
    endforeach()
endif()

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
    internal/arch-extract/tests
    dascov
    mcp
    lint
    detect-dupe
    find-dupe/tests
    gen1-to-gen2/tests
    internal/hygiene
    internal/preflight/tests
    internal/ast-fuzz
    benchctl/tests
    internal/requirefix
    internal/test-release
    daspkg/test_daspkg.das
)

FOREACH(_das ${DAS_UTILS_TO_TEST})
    LIST(APPEND _run_utils_tests
        COMMAND ${CMAKE_COMMAND} -E echo "Running tests ${_das}"
        COMMAND daslang ${PROJECT_SOURCE_DIR}/dastest/dastest.das -- --color --failures-only --test ${PROJECT_SOURCE_DIR}/utils/${_das}
        DEPENDS tree_sitter_daslang
    )
ENDFOREACH()

add_das_target(run_utils_tests
    ${_run_utils_tests}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running utils tests"
)

add_das_target(check_standalone_exes
    COMMAND $<TARGET_FILE:daslang> -exe -output ${PROJECT_SOURCE_DIR}/bin/das-fmt ./utils/das-fmt/dasfmt.das
    COMMAND $<TARGET_FILE:daslang> -exe -output ${PROJECT_SOURCE_DIR}/bin/das-lint ./utils/lint/main.das
    DEPENDS daslang all_utils_exe
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Building the standalone das-fmt and das-lint executables"
)
add_custom_target(check_format_exe
    COMMAND ${PROJECT_SOURCE_DIR}/bin/das-fmt.exe --path ${PROJECT_SOURCE_DIR}/ --verify --exclude-mask ${CMAKE_BINARY_DIR}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Verifying .das formatting with the standalone das-fmt"
)
add_dependencies(check_format_exe check_standalone_exes)

if(TARGET dascov_exe)
    add_das_target(run_coverage
        COMMAND $<TARGET_FILE:daslang> ${PROJECT_SOURCE_DIR}/dastest/dastest.das -- --cov-path ${CMAKE_CURRENT_BINARY_DIR}/coverage.lcov --color --failures-only --test ${PROJECT_SOURCE_DIR}/tests/language --timeout 1800
        COMMAND ${UTIL_BIN_DIR}/dascov.exe -- ${CMAKE_CURRENT_BINARY_DIR}/coverage.lcov --exclude tests/language
        DEPENDS daslang dascov_exe
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Coverage of tests/language (dastest --cov-path + dascov)"
    )
endif()
