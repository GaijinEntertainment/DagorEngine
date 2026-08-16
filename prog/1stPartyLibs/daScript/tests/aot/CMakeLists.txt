### AOT test target
### Builds a standalone binary with AOT-compiled test files

SET(AOT_TESTING_FILES
    dastest/testing.das
)

# AOT all daslib .das files — catches codegen regressions for any module,
# not just those used in tests/. Files marked `options no_aot` are silently skipped.
FILE(GLOB AOT_DASLIB_MODULE_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "daslib/*.das")
# Exclude modules that genuinely break batched AOT:
#   aot_macro      — [simulate_macro] fails to simulate during AOT generation
#   profiler       — emits AOT C++ hitting an ambiguous __bit_set overload (aot.h)
#   profiler_boost — its [_macro] forks profiler's thread-local instrumenting agent, which
#                    perturbs later batch files' hashes → error[50101]
#   network        — daslang class `Server` shadows C++ `das::Server` (server_init undeclared)
list(FILTER AOT_DASLIB_MODULE_FILES EXCLUDE REGEX "daslib/(aot_macro|profiler|profiler_boost|network)\\.das$")
# just_in_time bootstraps the LLVM-backed JIT module via [extern](library=LLVM.dll).
# Without DAS_LLVM_DISABLED=OFF, the [extern] macro fails at compile time.
IF(DAS_LLVM_DISABLED)
    list(FILTER AOT_DASLIB_MODULE_FILES EXCLUDE REGEX "daslib/just_in_time\\.das$")
ENDIF()

# Module .das sources are AOT-generation inputs too: an edit to a required module file (macro
# or runtime) changes dependents' semantic hashes, and a stale generated TU then fails the AOT
# link (error[50101]). DAS_AOT_EXTRA_DEPENDS wires the module globs into the affected dirs'
# regen commands (daslib/*.das is already a global dependency inside DAS_AOT_EXT).
SET(DAS_AOT_DASSPIRV_MODULE_DEPENDS ${DASSPIRV_AOT_DEPENDS})
SET(DAS_AOT_DASMETAL_MODULE_DEPENDS ${DASMETAL_AOT_DEPENDS})

# Module-lib AOT sources are owned by each module (set in modules/<M>/CMakeLists.txt, empty when
# the module is disabled). Bridge them to the suite-named vars the loops below consume. spirv/msl
# also pull one test-side fixture; dasMetal/dasSpirv are always enabled so no gate is needed.
SET(AOT_STBIMAGE_MODULE_FILES ${DASSTBIMAGE_AOT_FILES})
SET(AOT_SPIRV_MODULE_FILES ${DASSPIRV_AOT_FILES} tests/spirv/_spirv_common.das)
SET(AOT_MSL_MODULE_FILES ${DASMETAL_AOT_FILES} tests/msl/_msl_common.das)

FILE(GLOB AOT_TESTS_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/aot/*.das")
# Exclude _-prefixed helper modules (required transitively, not standalone test entry points)
list(FILTER AOT_TESTS_FILES EXCLUDE REGEX "/_")

FILE(GLOB AOT_BARE_BLOCK_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/bare_block/*.das")
list(FILTER AOT_BARE_BLOCK_FILES EXCLUDE REGEX "failed_|cant_|invalid_")

FILE(GLOB AOT_CLASS_BOOST_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/class_boost/*.das")
# Exclude expect-error tests (they intentionally fail to compile)
list(FILTER AOT_CLASS_BOOST_FILES EXCLUDE REGEX "failed_")

FILE(GLOB AOT_DASPEG_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/dasPEG/*.das")

FILE(GLOB AOT_DASGLTF_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/dasGLTF/*.das")

# dasLLAMA IS NEVER AOT'D. No dasLLAMA test — old or new — joins test_aot: the engine is
# -jit only (its AOT story ends at the generic compile of the tree), and its require graph
# reaches modules that live in SEPARATE REPOSITORIES (dasVulkan), whose AOT TUs cannot build
# from this one. Do not re-register it here.

FILE(GLOB AOT_DECS_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/decs/*.das")
# Exclude expect-error tests (they intentionally fail to compile)
# Exclude test_stages_extra (it is a module, compiled via DAS_AOT_LIB below)
list(FILTER AOT_DECS_FILES EXCLUDE REGEX "failed_|test_stages_extra")

# Decs test module files (libraries required by decs tests)
SET(AOT_DECS_MODULE_FILES
    tests/decs/test_stages_extra.das
)

# dasLiveHost tests transitively require dashv, so AOT them only when HV is enabled.
SET(AOT_LIVE_HOST_FILES)
SET(AOT_LIVE_HOST_MODULE_FILES ${DASLIVEHOST_AOT_FILES})
IF(NOT DAS_HV_DISABLED)
    FILE(GLOB AOT_LIVE_HOST_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/live_host/*.das")
ENDIF()

SET(AOT_DASSQLITE_MODULE_FILES ${DASSQLITE_AOT_FILES})

SET(AOT_DASSQLITE_FILES)
IF(NOT DAS_SQLITE_DISABLED)
    FILE(GLOB AOT_DASSQLITE_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/dasSQLITE/*.das")
    # Exclude expect-error tests (they intentionally fail to compile)
    list(FILTER AOT_DASSQLITE_FILES EXCLUDE REGEX "failed_")
ENDIF()

SET(AOT_SQL_CONFORMANCE_MODULE_FILES)
SET(AOT_SQL_CONFORMANCE_FILES)
IF(NOT DAS_SQLITE_DISABLED)
    SET(AOT_SQL_CONFORMANCE_MODULE_FILES
        tests/sql_conformance/_conformance_provider.das
    )
    FILE(GLOB AOT_SQL_CONFORMANCE_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/sql_conformance/*.das")
    # Exclude the shim module (AOT'd via DAS_AOT_LIB above)
    list(FILTER AOT_SQL_CONFORMANCE_FILES EXCLUDE REGEX "/_")
ENDIF()

SET(AOT_STRUDEL_MODULE_FILES ${DASAUDIO_AOT_FILES})

# audio-independent strudel test always AOT'd; audio-dependent ones need dasAudio (may be off in CI).
SET(AOT_STRUDEL_FILES
    tests/strudel/test_midi.das
)
IF(NOT DAS_AUDIO_DISABLED)
    LIST(APPEND AOT_STRUDEL_FILES
        tests/strudel/test_mini.das
        tests/strudel/test_pattern.das
        tests/strudel/test_midi_player.das
        tests/strudel/test_midi_extended.das
        tests/strudel/test_samples.das
        tests/strudel/test_sequencer.das
        tests/strudel/test_sf2.das
        tests/strudel/test_sf2_voice.das
        tests/strudel/test_sf2_modulators.das
        tests/strudel/test_adsr_resolver.das
        tests/strudel/test_aliases.das
        tests/strudel/test_chorus_per_orbit.das
        tests/strudel/test_combinators.das
        tests/strudel/test_delay.das
        tests/strudel/test_effects.das
        tests/strudel/test_fx_routing.das
        tests/strudel/test_new_features.das
        tests/strudel/test_numeric_widening.das
        tests/strudel/test_hrtf_pos.das
        tests/strudel/test_new_combinators.das
        tests/strudel/test_integration.das
        tests/strudel/test_memory.das
        tests/strudel/test_orbits.das
        tests/strudel/test_orbit_gain.das
        tests/strudel/test_reverb.das
        tests/strudel/test_scales.das
        tests/strudel/test_setters.das
        tests/strudel/test_signals.das
        tests/strudel/test_synthesis.das
        tests/strudel/test_vowel.das
        tests/strudel/test_sfxr.das
        tests/strudel/test_modal.das
        tests/strudel/test_sfx.das
        tests/strudel/test_one_shot.das
        tests/audio/test_hrtf_budget.das
        tests/audio/test_wav.das
    )
ENDIF()

SET(AOT_PUGIXML_MODULE_FILES ${DASPUGIXML_AOT_FILES})

SET(AOT_PUGIXML_FILES)
IF(NOT DAS_PUGIXML_DISABLED)
    FILE(GLOB AOT_PUGIXML_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/dasPUGIXML/*.das")
ENDIF()

SET(AOT_DASHV_MODULE_FILES ${DASHV_AOT_FILES})
IF(DASHV_AOT_FILES)
    LIST(APPEND AOT_DASHV_MODULE_FILES tests/dasHV/_dashv_test_common.das)
ENDIF()

SET(AOT_DASHV_FILES)
IF(NOT DAS_HV_DISABLED)
    FILE(GLOB AOT_DASHV_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/dasHV/*.das")
    # Exclude module files (compiled via DAS_AOT_LIB)
    list(FILTER AOT_DASHV_FILES EXCLUDE REGEX "/_")
ENDIF()

IF(NOT DAS_CLIPBOARD_DISABLED)
  FILE(GLOB AOT_CLIPBOARD_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/clipboard/*.das")
ELSE()
  SET(AOT_CLIPBOARD_FILES)
ENDIF()

FILE(GLOB AOT_FLATTEN_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/flatten/*.das")
# Exclude the _err_ boundary fixtures (they intentionally fail to compile) and the
# flat_api_macro helper module (a compile-time-only macro, not a test entry point)
list(FILTER AOT_FLATTEN_FILES EXCLUDE REGEX "_err_|flat_api_macro")

FILE(GLOB AOT_INTERFACES_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/interfaces/*.das")
# Exclude expect-error tests and AOT codegen failures
list(FILTER AOT_INTERFACES_FILES EXCLUDE REGEX "failed_|test_missing_")

FILE(GLOB AOT_LINQ_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/linq/*.das")
# Exclude module files (they are compiled via DAS_AOT_LIB below) and
# expect-error tests (use `expect` to assert intentional macro_failed
# diagnostics; AOT cannot compile them)
list(FILTER AOT_LINQ_FILES EXCLUDE REGEX "/_|failed_")

# Linq test module files (libraries required by linq tests)
SET(AOT_LINQ_MODULE_FILES
    tests/linq/_common.das
)

FILE(GLOB AOT_READER_MACRO_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/reader_macro/*.das")
# Exclude the inline reader-macro module (no_aot, compile-time only; required transitively by the test).
# Anchor on '/' so this does not also drop test_inline_reader.das (which ends in inline_reader.das).
list(FILTER AOT_READER_MACRO_FILES EXCLUDE REGEX "/inline_reader\\.das$")

FILE(GLOB AOT_MACRO_CALL_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/macro_call/*.das")
# Exclude helpers (files prefixed with `_`); they are required transitively
# by the actual test files and don't need standalone AOT entries.
list(FILTER AOT_MACRO_CALL_FILES EXCLUDE REGEX "/_")

FILE(GLOB AOT_MACRO_BOOST_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/macro_boost/*.das")
# Exclude the call_macro probe helper (prefixed with `_`); it's required transitively
# by the actual test file.
list(FILTER AOT_MACRO_BOOST_FILES EXCLUDE REGEX "/_")

FILE(GLOB AOT_WITH_BOOST_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/with_boost/*.das")
# Exclude failed_* expected-failure compile tests — they're not AOT-able.
list(FILTER AOT_WITH_BOOST_FILES EXCLUDE REGEX "/failed_")

# Macro_boost test module files (probe call_macro required transitively by tests)
SET(AOT_MACRO_BOOST_MODULE_FILES
    tests/macro_boost/_has_sideeffects_probe.das
)

FILE(GLOB AOT_MATH_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/math/*.das")
# Exclude module file (macro module, not a test)
list(FILTER AOT_MATH_FILES EXCLUDE REGEX "fake_numeric")

# Math test module files (libraries required by math tests)
SET(AOT_MATH_MODULE_FILES
    tests/math/fake_numeric.das
)

FILE(GLOB AOT_STRINGS_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/strings/*.das")
# Exclude expect-error tests
list(FILTER AOT_STRINGS_FILES EXCLUDE REGEX "failed_")

FILE(GLOB AOT_QUOTE_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/quote/*.das")
# Exclude fixture modules (compiled via DAS_AOT_LIB below)
list(FILTER AOT_QUOTE_FILES EXCLUDE REGEX "/_")

# Quote test module files (A/B fixtures required by the quote tests)
SET(AOT_QUOTE_MODULE_FILES
    tests/quote/_quote_entities.das
    tests/quote/_quote_shapes.das
    tests/quote/_quote_shapes_lowered.das
)

FILE(GLOB AOT_STBIMAGE_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/stbimage/*.das")
# Exclude gen_test_images (utility script, not a test)
list(FILTER AOT_STBIMAGE_FILES EXCLUDE REGEX "gen_test_images")

# StbImage test module files (libraries required by stbimage tests)

SET(AOT_MINFFT_FILES)
IF(NOT DAS_MINFFT_DISABLED)
    FILE(GLOB AOT_MINFFT_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/minfft/*.das")
ENDIF()

# no module-AOT list needed (they call no runtime module fns, only `find` + dastest, already stubbed).

FILE(GLOB AOT_SPIRV_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/spirv/*.das")
# Exclude the shared fixture module (compiled via DAS_AOT_LIB below)
list(FILTER AOT_SPIRV_FILES EXCLUDE REGEX "/_")

# Spirv module + test-fixture files, AOT-compiled as libraries (their runtime functions link
# into test_aot so the tests' stubs resolve). The runtime spirv modules MUST be here — without
# them the tests' transitive builtin-generic instantiations (array/table finalize, to_array_move,
# resize, push_from) namespaced to spirv_builder have no stub → error[50101]. The macro-only
# modules (spirv_emit / spirv_shader) are NOT listed: they run at compile time and contribute no
# runtime functions. Dependency order (grammar → builder → types → dis → builtins → fixture).
# The fixture is _spirv_common (not _common) to avoid an AOT-lib symbol clash with
# tests/linq/_common.das (both would emit das::impl_aot__common → LNK2005 in test_aot).

# fixture module is AOT-compiled as a library below. The macro-only metal modules (msl_types /
# msl_emit / msl_shader) are NOT listed: they run at compile time and contribute no runtime
# functions (spirv_emit precedent).
FILE(GLOB AOT_MSL_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/msl/*.das")
list(FILTER AOT_MSL_FILES EXCLUDE REGEX "/_")

# requires (`require ?das_metal`) make the files compile everywhere — on non-Apple platforms
# they reduce to the CPU-reference half, and the stubs generated there match that reduction.
# The shared fixture module (generic GPU helpers) is AOT-compiled as a library below.
FILE(GLOB AOT_METAL_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/metal/*.das")
list(FILTER AOT_METAL_FILES EXCLUDE REGEX "/_")
SET(AOT_METAL_MODULE_FILES
    tests/metal/_metal_common.das
    # f16_cvt is a generic dasLLVM daslib helper the tensor-ops test requires; it used to ride
    # the (deleted) dasLLAMA module lib — dasLLAMA itself is NEVER AOT'd (see above)
    modules/dasLLVM/daslib/f16_cvt.das
)
IF(APPLE)
    # das_metal_boost hard-requires the APPLE-gated das_metal C++ module; the GPU-behavioral
    # halves only exist on Apple (`require ?das_metal` + static_if feints elsewhere)
    LIST(APPEND AOT_METAL_MODULE_FILES modules/dasMetal/metal/das_metal_boost.das)
ENDIF()

SET(AOT_DASLIB_FILES
    tests/daslib/clargs_test.das
    tests/daslib/test_toml.das
    tests/daslib/test_logger.das
    tests/daslib/tty_test.das
    tests/daslib/test_sha_256.das
)

FILE(GLOB AOT_MCP_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/mcp/*.das")
# Skip _fixture_*.das — those are subprocess targets spawned BY the tests,
# not test entry points; AOT'ing them would pull in get_command_line_arguments
# at a fixture that doesn't need it and bloats the build for no coverage.
list(FILTER AOT_MCP_FILES EXCLUDE REGEX "/_")

FILE(GLOB AOT_LSP_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/lsp/*.das")
# Skip _fixture_*.das — opened THROUGH the supervisor by the tests, not test entry points
list(FILTER AOT_LSP_FILES EXCLUDE REGEX "/_")

FILE(GLOB AOT_LINT_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/lint/*.das")
# Exclude _*.das fixture files (consumed at runtime via compile_file, not invoked by dastest)
list(FILTER AOT_LINT_FILES EXCLUDE REGEX "/_")
# Exclude failed_*.das fixtures (expect compile errors that AOT codegen can't process)
list(FILTER AOT_LINT_FILES EXCLUDE REGEX "/failed_")

FILE(GLOB AOT_PROMOTE_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/promote/*.das")
# Exclude expect-error tests (range overflow doesn't AOT)
list(FILTER AOT_PROMOTE_FILES EXCLUDE REGEX "failed_")

FILE(GLOB AOT_RTTI_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/rtti/*.das")
list(FILTER AOT_RTTI_FILES EXCLUDE REGEX "/_")

FILE(GLOB AOT_FIXED_ARRAY_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/fixed_array/*.das")
# Exclude failed_* expect-fixtures and _-prefixed target-spec files (enabled at Stage 1)
list(FILTER AOT_FIXED_ARRAY_FILES EXCLUDE REGEX "failed_|/_")

FILE(GLOB AOT_TYPEMACRO_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/typemacro/*.das")
# Exclude the _-prefixed macro modules (compile-time only, options no_aot) and failed_* expect-fixtures
list(FILTER AOT_TYPEMACRO_FILES EXCLUDE REGEX "failed_|/_")

FILE(GLOB AOT_LANGUAGE_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/language/*.das")
# Exclude expect-error tests and no_aot files
list(FILTER AOT_LANGUAGE_FILES EXCLUDE REGEX "failed_|cant_|invalid_")
# Exclude module files (they are compiled via DAS_AOT_LIB below)
list(FILTER AOT_LANGUAGE_FILES EXCLUDE REGEX "/_")

# Minimal daslib closure for test_aot_subset: ONLY the daslib modules whose
# runtime functions tests/language actually calls (50101 otherwise). This is
# NOT the all-daslib coverage lib above — keep it minimal; a new tests/language
# test requiring another runtime daslib module extends this list. Leaf-first.
SET(AOT_SUBSET_DASLIB_MODULE_FILES
    daslib/math_bits.das
    daslib/random.das
    daslib/defer.das
    daslib/strings_boost.das
    daslib/enum_trait.das
    daslib/regex.das
    daslib/regex_boost.das
    daslib/static_let.das
    daslib/faker.das
    daslib/fuzzer.das
    daslib/coroutines.das
    daslib/contracts.das
    daslib/apply.das
    daslib/templates.das
    daslib/rtti.das
    daslib/archive.das
)

# Language test module files (libraries required by tests)
SET(AOT_LANGUAGE_MODULE_FILES
    tests/language/_distinct_fixture.das
    tests/language/_glob.das
    tests/language/_helper_foo.das
    tests/language/_inline_constant_expression_helper.das
    tests/language/_inline_counter_helper.das
    tests/language/_inline_generic_helper.das
    tests/language/_inline_scope_helper.das
    tests/language/_inline_unsafe_helper.das
    tests/language/_lambda_vis_inner.das
    tests/language/_lambda_vis_mid.das
    tests/language/_never_inline_module.das
    tests/language/_operators_derived.das
    tests/language/_operators_parent.das
    tests/language/_string_clone_init_helper.das
)

# Single source of truth for the per-suite AOT scaffolding. Two loops build the uniform suites;
# a third, data-driven loop builds the irregular ones (per-module IF guards, shader-generator
# EXTRA_DEPENDS, the lib-only testing suite, and the subset-only daslib modules). All accumulate
# TEST_AOT_TARGETS / TEST_AOT_GENVARS, reused below for source groups, the test_aot executable
# sources, and its dependencies (subset-only rows do not, acc=0).
set(TEST_AOT_TARGETS "")
set(TEST_AOT_GENVARS "")
set(TEST_AOT_ALL_DAS "")   # test-body .das (not module libs); LLVM-AOT corpus emits each whole-program, covering its deps

set(DAS_AOT_SUITES
    algorithm apply archive assert_once ast_match async bare_block base64 bitfields bool_array
    class_boost daslib dasgltf daspeg data_walker debug debug_agent decs dynamic_cast_rtti fio
    fixed_array flatten fs functional gc glsl handle_types hash_map interfaces jit
    jobque json jsonrpc language linq lint long_array_table loops lpipe lsp
    macro_boost macro_call match math mcp md_boost module_tests option promote quote
    reader_macro regex rtti safe_addr soa spoof strings stbimage table_packed template
    tests type_lattice type_traits typemacro uri with_boost delegate)
foreach(_s IN LISTS DAS_AOT_SUITES)
    string(TOUPPER ${_s} _u)
    # suites with an irregular dir / a filter / a curated list define AOT_<UC>_FILES above;
    # the rest glob tests/<name>/*.das by convention here.
    if(NOT DEFINED AOT_${_u}_FILES)
        file(GLOB AOT_${_u}_FILES RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "tests/${_s}/*.das")
    endif()
    add_custom_target(test_aot_${_s})
    set(${_u}_AOT_GENERATED_SRC)
    list(LENGTH AOT_${_u}_FILES _nf)
    if(_nf GREATER 0)
        DAS_AOT("${AOT_${_u}_FILES}" ${_u}_AOT_GENERATED_SRC test_aot_${_s} daslang)
    endif()
    list(APPEND TEST_AOT_TARGETS test_aot_${_s})
    list(APPEND TEST_AOT_GENVARS ${_u}_AOT_GENERATED_SRC)
    list(APPEND TEST_AOT_ALL_DAS ${AOT_${_u}_FILES})
endforeach()

set(DAS_AOT_MODULE_SUITES daslib decs language linq macro_boost math quote stbimage)
foreach(_s IN LISTS DAS_AOT_MODULE_SUITES)
    string(TOUPPER ${_s} _u)
    add_custom_target(test_aot_${_s}_modules)
    set(${_u}_MODULES_AOT_GENERATED_SRC)
    list(LENGTH AOT_${_u}_MODULE_FILES _nf)
    if(_nf GREATER 0)
        DAS_AOT_LIB("${AOT_${_u}_MODULE_FILES}" ${_u}_MODULES_AOT_GENERATED_SRC test_aot_${_s}_modules daslang)
    endif()
    list(APPEND TEST_AOT_TARGETS test_aot_${_s}_modules)
    list(APPEND TEST_AOT_GENVARS ${_u}_MODULES_AOT_GENERATED_SRC)
endforeach()

# one row per irregular suite:  name | flavor(reg/mod/lib) | guard(- none) | EXTRA_DEPENDS vars(- none) | accumulate(1/0)
set(DAS_AOT_IRREGULAR
    "testing|lib|-|-|1"
    "spirv|reg|-|DAS_AOT_DASSPIRV_MODULE_DEPENDS|1"
    "spirv|mod|-|DAS_AOT_DASSPIRV_MODULE_DEPENDS|1"
    "msl|reg|-|DAS_AOT_DASMETAL_MODULE_DEPENDS|1"
    "msl|mod|-|DAS_AOT_DASMETAL_MODULE_DEPENDS|1"
    "metal|reg|-|DAS_AOT_DASMETAL_MODULE_DEPENDS|1"
    "metal|mod|-|DAS_AOT_DASMETAL_MODULE_DEPENDS|1"
    "live_host|reg|NOT DAS_HV_DISABLED|-|1"
    "live_host|mod|-|-|1"
    "strudel|reg|NOT DAS_AUDIO_DISABLED|-|1"
    "strudel|mod|-|-|1"
    "dassqlite|reg|NOT DAS_SQLITE_DISABLED|-|1"
    "dassqlite|mod|-|-|1"
    "sql_conformance|reg|NOT DAS_SQLITE_DISABLED|-|1"
    "sql_conformance|mod|NOT DAS_SQLITE_DISABLED|-|1"
    "pugixml|reg|NOT DAS_PUGIXML_DISABLED|-|1"
    "pugixml|mod|-|-|1"
    "dashv|reg|NOT DAS_HV_DISABLED|-|1"
    "dashv|mod|-|-|1"
    "clipboard|reg|AOT_CLIPBOARD_FILES|-|1"
    "minfft|reg|NOT DAS_MINFFT_DISABLED|-|1"
    "subset_daslib|mod|-|-|0"
)
foreach(_row IN LISTS DAS_AOT_IRREGULAR)
    string(REPLACE "|" ";" _c "${_row}")
    list(GET _c 0 _name)
    list(GET _c 1 _flav)
    list(GET _c 2 _guard)
    list(GET _c 3 _extra)
    list(GET _c 4 _acc)
    string(TOUPPER ${_name} _u)
    if(_flav STREQUAL "mod")
        set(_tgt test_aot_${_name}_modules)
        set(_files AOT_${_u}_MODULE_FILES)
        set(_gen ${_u}_MODULES_AOT_GENERATED_SRC)
    else()
        set(_tgt test_aot_${_name})
        set(_files AOT_${_u}_FILES)
        set(_gen ${_u}_AOT_GENERATED_SRC)
    endif()
    add_custom_target(${_tgt})
    set(${_gen})
    set(DAS_AOT_EXTRA_DEPENDS "")
    if(NOT _extra STREQUAL "-")
        string(REPLACE " " ";" _evars "${_extra}")
        foreach(_ev IN LISTS _evars)
            list(APPEND DAS_AOT_EXTRA_DEPENDS ${${_ev}})
        endforeach()
    endif()
    set(_emit TRUE)
    if(NOT _guard STREQUAL "-")
        # _guard is ONE list element ("NOT DAS_X_DISABLED") — expanded as a single argument,
        # if() reads the whole string as an (undefined) variable name and every guarded row
        # silently stops emitting. Split into real tokens first.
        string(REPLACE " " ";" _guard_expr "${_guard}")
        if(NOT (${_guard_expr}))
            set(_emit FALSE)
        endif()
    endif()
    list(LENGTH ${_files} _nf)
    if(_emit AND _nf GREATER 0)
        if(_flav STREQUAL "reg")
            DAS_AOT("${${_files}}" ${_gen} ${_tgt} daslang)
        else()
            DAS_AOT_LIB("${${_files}}" ${_gen} ${_tgt} daslang)
        endif()
    endif()
    set(DAS_AOT_EXTRA_DEPENDS "")
    if(_acc STREQUAL "1")
        list(APPEND TEST_AOT_TARGETS ${_tgt})
        list(APPEND TEST_AOT_GENVARS ${_gen})
        if(_flav STREQUAL "reg")
            list(APPEND TEST_AOT_ALL_DAS ${${_files}})
        endif()
    endif()
endforeach()

foreach(_v IN LISTS TEST_AOT_GENVARS)
    SOURCE_GROUP_FILES("aot generated" ${_v})
endforeach()

# Build the test_aot executable using the same main.cpp as daslang.
# EXCLUDE_FROM_ALL: the full binary (~1080 AOT TUs + heavy link) is built only
# where it's explicitly requested — the nightly CI cron and `preflight --full`
# (both via `--target test_aot` / `--target run_tests_aot`). Per-PR CI builds
# test_aot_subset below instead.
set(TEST_AOT_SRC "")
foreach(_v IN LISTS TEST_AOT_GENVARS)
    list(APPEND TEST_AOT_SRC ${${_v}})
endforeach()

add_executable(test_aot EXCLUDE_FROM_ALL ${DAS_DASCRIPT_MAIN_SRC}
    ${TEST_AOT_SRC}
)
TARGET_LINK_LIBRARIES(test_aot libDaScriptAot ${SRC_LIBRARIES} ${DAS_MODULES_LIBS})
ADD_DEPENDENCIES(test_aot libDaScriptAot
    ${TEST_AOT_TARGETS}
)
# Shared between test_aot and test_aot_subset — the two binaries MUST compile
# main.cpp identically (a define present on one but not the other silently
# changes module registration between the per-PR gate and the nightly binary).
SET(TEST_AOT_INCLUDE_DIRS ${NEED_MODULES_PATH} ${PROJECT_SOURCE_DIR}/modules/dasUnitTest ${PROJECT_SOURCE_DIR}/modules/dasLiveHost/src ${AUDIO_INCLUDE_DIR} ${CIPIC_HRTF_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/modules/dasMinfft/minfft ${PROJECT_SOURCE_DIR}/modules/dasPUGIXML/pugixml ${PROJECT_SOURCE_DIR}/modules/dasHV/hv/$<CONFIG>/include ${SQLITE_INCLUDE_DIR})
SET(TEST_AOT_COMPILE_DEFS DAS_TEST_AOT MINFFT_SINGLE HRTF_SAMPLE_RATE=${DAS_AUDIO_SAMPLE_RATE})

target_include_directories(test_aot PRIVATE ${TEST_AOT_INCLUDE_DIRS})
target_compile_definitions(test_aot PRIVATE ${TEST_AOT_COMPILE_DEFS})
SETUP_CPP11(test_aot)
# No SETUP_LTO: LTCG over ~1080 AOT TUs was a 21-minute link on MSVC CI for a
# test binary that measures correctness, not performance.

### AOT subset target — the per-PR CI gate
# tests/language only (+ dastest/testing.das and the language fixture modules).
# Part of the default build on every lane, Debug included: a fast compile+link
# gate (~230 TUs, no LTO). The full suite builds and runs on the nightly cron.
add_executable(test_aot_subset ${DAS_DASCRIPT_MAIN_SRC}
    ${TESTING_AOT_GENERATED_SRC}
    ${LANGUAGE_AOT_GENERATED_SRC}
    ${LANGUAGE_MODULES_AOT_GENERATED_SRC}
    ${SUBSET_DASLIB_MODULES_AOT_GENERATED_SRC}
)
TARGET_LINK_LIBRARIES(test_aot_subset libDaScriptAot ${SRC_LIBRARIES} ${DAS_MODULES_LIBS})
ADD_DEPENDENCIES(test_aot_subset libDaScriptAot
    test_aot_testing
    test_aot_language test_aot_language_modules
    test_aot_subset_daslib_modules
)
target_include_directories(test_aot_subset PRIVATE ${TEST_AOT_INCLUDE_DIRS})
target_compile_definitions(test_aot_subset PRIVATE ${TEST_AOT_COMPILE_DEFS})
SETUP_CPP11(test_aot_subset)

### LLVM-AOT variant of test_aot (LLVM-only). Each .das is compiled by the LLVM backend into a
### self-registering native .o (das_aot_register load ctor) linked directly into the binary;
### -use-aot binds each function as a SimNode_Jit via linkCppAot. Opt-in, not in ALL / not in CI.
if(NOT ${DAS_LLVM_DISABLED})
    # Corpus: the jit_tests suite -- files that JIT cleanly by construction (the -jit lane's inputs).
    # Cover exactly what C++ AOT (test_aot) covers — the accumulated TEST_AOT_ALL_DAS (every suite's
    # test bodies + module libs) — plus the jit_tests corpus, so `run_tests_llvm_aot --test tests/`
    # binds every function (a miss is a hard error under --use-aot). The JIT backend already compiles
    # all of tests/ (run_tests_jit is green), so --aot-object emits them too.
    FILE(GLOB _llvm_aot_jit_tests RELATIVE ${PROJECT_SOURCE_DIR} CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/tests/jit_tests/*.das")
    set(LLVM_AOT_TEST_FILES ${TEST_AOT_ALL_DAS} ${_llvm_aot_jit_tests})
    list(REMOVE_DUPLICATES LLVM_AOT_TEST_FILES)
    # _-prefixed helpers and negative/fixture tests.
    list(FILTER LLVM_AOT_TEST_FILES EXCLUDE REGEX "/_")
    # jit_fastpath (is_jit_function/remove_jit semantics) and typeinfo behave differently under
    # SimNode_Jit binding; llvm_code/compile-only are not object-emitted. Everything else is covered.
    list(FILTER LLVM_AOT_TEST_FILES EXCLUDE REGEX "/(cant_|llvm_tune|llvm_code|llvm_compile_only|dll_cache|jit_fastpath|typeinfo)|_solid\\.das$")
    # msl/metal exercise 16/8-bit lattice + simdgroup ops the LLVM JIT declines (lattice_fallback in
    # llvm_jit.das) — they interpret, so they have no emitted object; keep them out of the AOT rail.
    list(FILTER LLVM_AOT_TEST_FILES EXCLUDE REGEX "tests/(msl|metal)/")

    add_custom_target(test_llvm_aot_corpus)
    SET(LLVM_AOT_GENERATED_OBJ)
    DAS_LLVM_AOT_LIB("${LLVM_AOT_TEST_FILES}" LLVM_AOT_GENERATED_OBJ test_llvm_aot_corpus)

    # Same main.cpp as daslang / test_aot; the .o are pre-built external inputs (EXTERNAL_OBJECT),
    # so the C++ toolchain links them without recompiling.
    add_executable(test_llvm_aot EXCLUDE_FROM_ALL ${DAS_DASCRIPT_MAIN_SRC} ${LLVM_AOT_GENERATED_OBJ})
    TARGET_LINK_LIBRARIES(test_llvm_aot libDaScriptAot ${SRC_LIBRARIES} ${DAS_MODULES_LIBS})
    ADD_DEPENDENCIES(test_llvm_aot libDaScriptAot test_llvm_aot_corpus)
    target_include_directories(test_llvm_aot PRIVATE ${NEED_MODULES_PATH} ${PROJECT_SOURCE_DIR}/modules/dasUnitTest)
    target_compile_definitions(test_llvm_aot PRIVATE DAS_TEST_AOT)
    SETUP_CPP11(test_llvm_aot)
    SETUP_LTO(test_llvm_aot)
endif()
