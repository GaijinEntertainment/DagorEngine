### AOT test target
### Builds a standalone binary with AOT-compiled test files

# AOT for dastest/testing.das (library module used by all tests)
SET(AOT_TESTING_FILES
    dastest/testing.das
)

add_custom_target(test_aot_testing)
SET(TESTING_AOT_GENERATED_SRC)
DAS_AOT_LIB("${AOT_TESTING_FILES}" TESTING_AOT_GENERATED_SRC test_aot_testing daslang)

# AOT for daslib modules (library dependencies used by test files)
SET(AOT_DASLIB_MODULE_FILES
    daslib/archive.das
    daslib/ast_boost.das
    daslib/ast_cursor.das
    daslib/ast_match.das
    daslib/base64.das
    daslib/bool_array.das
    daslib/coroutines.das
    daslib/debug_eval.das
    daslib/decs.das
    daslib/faker.das
    daslib/fuzzer.das
    daslib/json.das
    daslib/json_boost.das
    daslib/linq.das
    daslib/linq_boost.das
    daslib/lpipe.das
    daslib/match.das
    daslib/math_bits.das
    daslib/random.das
    daslib/regex.das
    daslib/regex_boost.das
    daslib/safe_addr.das
    daslib/soa.das
    daslib/spoof.das
    daslib/linked_list.das
    daslib/strings_boost.das
    daslib/temp_strings.das
    daslib/templates.das
    daslib/templates_boost.das
    daslib/type_traits.das
    daslib/bitfield_trait.das
    daslib/defer.das
    daslib/uriparser_boost.das
)

add_custom_target(test_aot_daslib_modules)
SET(DASLIB_MODULES_AOT_GENERATED_SRC)
DAS_AOT_LIB("${AOT_DASLIB_MODULE_FILES}" DASLIB_MODULES_AOT_GENERATED_SRC test_aot_daslib_modules daslang)

# AOT for individual test files
FILE(GLOB AOT_TEST_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/aot/*.das")

# AOT for algorithm test files
FILE(GLOB AOT_ALGORITHM_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/algorithm/*.das")

# AOT for apply test files
FILE(GLOB AOT_APPLY_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/apply/*.das")

# AOT for archive test files
FILE(GLOB AOT_ARCHIVE_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/archive/*.das")

# AOT for ast_match test files
FILE(GLOB AOT_AST_MATCH_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/ast_match/*.das")

# AOT for assert_once test files
FILE(GLOB AOT_ASSERT_ONCE_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/assert_once/*.das")

# AOT for async test files
FILE(GLOB AOT_ASYNC_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/async/*.das")

# AOT for bare_block test files
FILE(GLOB AOT_BARE_BLOCK_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/bare_block/*.das")

# AOT for base64 test files
FILE(GLOB AOT_BASE64_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/base64/*.das")

# AOT for bitfields test files
FILE(GLOB AOT_BITFIELDS_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/bitfields/*.das")

# AOT for bool_array test files
FILE(GLOB AOT_BOOL_ARRAY_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/bool_array/*.das")

# AOT for data_walker test files
FILE(GLOB AOT_DATA_WALKER_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/data_walker/*.das")

# AOT for debug test files
FILE(GLOB AOT_DEBUG_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/debug/*.das")

# AOT for debug_agent test files
FILE(GLOB AOT_DEBUG_AGENT_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/debug_agent/*.das")

# AOT for decs test files
FILE(GLOB AOT_DECS_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/decs/*.das")
# Exclude expect-error tests (they intentionally fail to compile)
# Exclude test_stages_extra (it is a module, compiled via DAS_AOT_LIB below)
list(FILTER AOT_DECS_FILES EXCLUDE REGEX "failed_|test_stages_extra")

# Decs test module files (libraries required by decs tests)
SET(AOT_DECS_MODULE_FILES
    tests/decs/test_stages_extra.das
)

# AOT for live_host test files
FILE(GLOB AOT_LIVE_HOST_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/live_host/*.das")

# Live host module files (libraries required by live_host tests)
SET(AOT_LIVE_HOST_MODULE_FILES
    modules/dasLiveHost/live/live_commands.das
    modules/dasLiveHost/live/live_vars.das
)

# AOT for strudel/audio module library files (required by strudel tests)
SET(AOT_STRUDEL_MODULE_FILES)
IF(NOT DAS_AUDIO_DISABLED)
    SET(AOT_STRUDEL_MODULE_FILES
        modules/dasAudio/audio/audio_boost.das
        modules/dasAudio/audio/audio_wav.das
        modules/dasAudio/strudel/strudel_event.das
        modules/dasAudio/strudel/strudel_time.das
        modules/dasAudio/strudel/strudel_pattern.das
        modules/dasAudio/strudel/strudel_mini.das
        modules/dasAudio/strudel/strudel_synth.das
        modules/dasAudio/strudel/strudel_samples.das
        modules/dasAudio/strudel/strudel_scheduler.das
        modules/dasAudio/strudel/strudel_sf2.das
        modules/dasAudio/strudel/strudel_sf2_voice.das
        modules/dasAudio/strudel/strudel_midi.das
        modules/dasAudio/strudel/strudel_midi_player.das
        modules/dasAudio/strudel/strudel_player.das
    )
ENDIF()

# AOT for strudel test files (audio-independent only; audio-dependent tests
# require dasAudio which may be disabled in CI)
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
    )
ENDIF()

# AOT for dasPUGIXML module library files
SET(AOT_PUGIXML_MODULE_FILES)
IF(NOT DAS_PUGIXML_DISABLED)
    SET(AOT_PUGIXML_MODULE_FILES
        modules/dasPUGIXML/daslib/PUGIXML_boost.das
    )
ENDIF()

# AOT for dasPUGIXML test files
SET(AOT_PUGIXML_FILES)
IF(NOT DAS_PUGIXML_DISABLED)
    FILE(GLOB AOT_PUGIXML_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/dasPUGIXML/*.das")
ENDIF()

# AOT for dasHV module library files
SET(AOT_DASHV_MODULE_FILES)
IF(NOT DAS_HV_DISABLED)
    SET(AOT_DASHV_MODULE_FILES
        modules/dasHV/dashv/dashv_boost.das
        tests/dasHV/_dashv_test_common.das
    )
ENDIF()

# AOT for dasHV test files
SET(AOT_DASHV_FILES)
IF(NOT DAS_HV_DISABLED)
    FILE(GLOB AOT_DASHV_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/dasHV/*.das")
    # Exclude module files (compiled via DAS_AOT_LIB)
    list(FILTER AOT_DASHV_FILES EXCLUDE REGEX "/_")
ENDIF()

# AOT for dynamic_cast_rtti test files
FILE(GLOB AOT_DYNAMIC_CAST_RTTI_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/dynamic_cast_rtti/*.das")

# AOT for fio test files
FILE(GLOB AOT_FIO_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/fio/*.das")

# AOT for fs test files
FILE(GLOB AOT_FS_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/fs/*.das")

# AOT for functional test files
FILE(GLOB AOT_FUNCTIONAL_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/functional/*.das")

# AOT for handle_types test files
FILE(GLOB AOT_HANDLE_TYPES_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/handle_types/*.das")

# AOT for hash_map test files
FILE(GLOB AOT_HASH_MAP_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/hash_map/*.das")

# AOT for interfaces test files
FILE(GLOB AOT_INTERFACES_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/interfaces/*.das")
# Exclude expect-error tests and AOT codegen failures
list(FILTER AOT_INTERFACES_FILES EXCLUDE REGEX "failed_|test_missing_")

# AOT for jobque test files
FILE(GLOB AOT_JOBQUE_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/jobque/*.das")

# AOT for json test files
FILE(GLOB AOT_JSON_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/json/*.das")

# AOT for linq test files
FILE(GLOB AOT_LINQ_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/linq/*.das")
# Exclude module files (they are compiled via DAS_AOT_LIB below)
list(FILTER AOT_LINQ_FILES EXCLUDE REGEX "/_")

# Linq test module files (libraries required by linq tests)
SET(AOT_LINQ_MODULE_FILES
    tests/linq/_common.das
)

# AOT for match test files
FILE(GLOB AOT_MATCH_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/match/*.das")

# AOT for math test files
FILE(GLOB AOT_MATH_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/math/*.das")
# Exclude module file (macro module, not a test)
list(FILTER AOT_MATH_FILES EXCLUDE REGEX "fake_numeric")

# Math test module files (libraries required by math tests)
SET(AOT_MATH_MODULE_FILES
    tests/math/fake_numeric.das
)

# AOT for module_tests test files
FILE(GLOB AOT_MODULE_TESTS_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/module_tests/*.das")

# AOT for regex test files
FILE(GLOB AOT_REGEX_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/regex/*.das")

# AOT for safe_addr test files
FILE(GLOB AOT_SAFE_ADDR_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/safe_addr/*.das")

# AOT for delegate test files
FILE(GLOB AOT_DELEGATE_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/delegate/*.das")

# AOT for soa test files
FILE(GLOB AOT_SOA_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/soa/*.das")

# AOT for spoof test files
FILE(GLOB AOT_SPOOF_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/spoof/*.das")

# AOT for strings test files
FILE(GLOB AOT_STRINGS_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/strings/*.das")
# Exclude expect-error tests
list(FILTER AOT_STRINGS_FILES EXCLUDE REGEX "_failed")

# AOT for template test files
FILE(GLOB AOT_TEMPLATE_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/template/*.das")

# AOT for type_traits test files
FILE(GLOB AOT_TYPE_TRAITS_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/type_traits/*.das")

# AOT for uri test files
FILE(GLOB AOT_URI_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/uri/*.das")

# AOT for lpipe test files
FILE(GLOB AOT_LPIPE_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/lpipe/*.das")

# AOT for stbimage test files
FILE(GLOB AOT_STBIMAGE_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/stbimage/*.das")
# Exclude gen_test_images (utility script, not a test)
list(FILTER AOT_STBIMAGE_FILES EXCLUDE REGEX "gen_test_images")

# StbImage test module files (libraries required by stbimage tests)
SET(AOT_STBIMAGE_MODULE_FILES
    modules/dasStbImage/stbimage/stbimage_boost.das
    modules/dasStbImage/stbimage/stbimage_ttf.das
)

# AOT for jit test files
FILE(GLOB AOT_JIT_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/jit/*.das")

# AOT for language test files
FILE(GLOB AOT_LANGUAGE_TEST_FILES RELATIVE ${PROJECT_SOURCE_DIR} "tests/language/*.das")
# Exclude expect-error tests and no_aot files
list(FILTER AOT_LANGUAGE_TEST_FILES EXCLUDE REGEX "failed_|cant_|invalid_")
# Exclude module files (they are compiled via DAS_AOT_LIB below)
list(FILTER AOT_LANGUAGE_TEST_FILES EXCLUDE REGEX "/_")

# Language test module files (libraries required by tests)
SET(AOT_LANGUAGE_MODULE_FILES
    tests/language/_glob.das
    tests/language/_helper_foo.das
    tests/language/_operators_derived.das
    tests/language/_operators_parent.das
)

add_custom_target(test_aot_algorithm)
SET(ALGORITHM_AOT_GENERATED_SRC)
DAS_AOT("${AOT_ALGORITHM_FILES}" ALGORITHM_AOT_GENERATED_SRC test_aot_algorithm daslang)

add_custom_target(test_aot_apply)
SET(APPLY_AOT_GENERATED_SRC)
DAS_AOT("${AOT_APPLY_FILES}" APPLY_AOT_GENERATED_SRC test_aot_apply daslang)

add_custom_target(test_aot_archive)
SET(ARCHIVE_AOT_GENERATED_SRC)
DAS_AOT("${AOT_ARCHIVE_FILES}" ARCHIVE_AOT_GENERATED_SRC test_aot_archive daslang)

add_custom_target(test_aot_ast_match)
SET(AST_MATCH_AOT_GENERATED_SRC)
DAS_AOT("${AOT_AST_MATCH_FILES}" AST_MATCH_AOT_GENERATED_SRC test_aot_ast_match daslang)

add_custom_target(test_aot_assert_once)
SET(ASSERT_ONCE_AOT_GENERATED_SRC)
DAS_AOT("${AOT_ASSERT_ONCE_FILES}" ASSERT_ONCE_AOT_GENERATED_SRC test_aot_assert_once daslang)

add_custom_target(test_aot_async)
SET(ASYNC_AOT_GENERATED_SRC)
DAS_AOT("${AOT_ASYNC_FILES}" ASYNC_AOT_GENERATED_SRC test_aot_async daslang)

add_custom_target(test_aot_bare_block)
SET(BARE_BLOCK_AOT_GENERATED_SRC)
DAS_AOT("${AOT_BARE_BLOCK_FILES}" BARE_BLOCK_AOT_GENERATED_SRC test_aot_bare_block daslang)

add_custom_target(test_aot_base64)
SET(BASE64_AOT_GENERATED_SRC)
DAS_AOT("${AOT_BASE64_FILES}" BASE64_AOT_GENERATED_SRC test_aot_base64 daslang)

add_custom_target(test_aot_bitfields)
SET(BITFIELDS_AOT_GENERATED_SRC)
DAS_AOT("${AOT_BITFIELDS_FILES}" BITFIELDS_AOT_GENERATED_SRC test_aot_bitfields daslang)

add_custom_target(test_aot_bool_array)
SET(BOOL_ARRAY_AOT_GENERATED_SRC)
DAS_AOT("${AOT_BOOL_ARRAY_FILES}" BOOL_ARRAY_AOT_GENERATED_SRC test_aot_bool_array daslang)

add_custom_target(test_aot_data_walker)
SET(DATA_WALKER_AOT_GENERATED_SRC)
DAS_AOT("${AOT_DATA_WALKER_FILES}" DATA_WALKER_AOT_GENERATED_SRC test_aot_data_walker daslang)

add_custom_target(test_aot_debug)
SET(DEBUG_AOT_GENERATED_SRC)
DAS_AOT("${AOT_DEBUG_FILES}" DEBUG_AOT_GENERATED_SRC test_aot_debug daslang)

add_custom_target(test_aot_debug_agent)
SET(DEBUG_AGENT_AOT_GENERATED_SRC)
DAS_AOT("${AOT_DEBUG_AGENT_FILES}" DEBUG_AGENT_AOT_GENERATED_SRC test_aot_debug_agent daslang)

add_custom_target(test_aot_decs)
SET(DECS_AOT_GENERATED_SRC)
DAS_AOT("${AOT_DECS_FILES}" DECS_AOT_GENERATED_SRC test_aot_decs daslang)

add_custom_target(test_aot_decs_modules)
SET(DECS_MODULES_AOT_GENERATED_SRC)
DAS_AOT_LIB("${AOT_DECS_MODULE_FILES}" DECS_MODULES_AOT_GENERATED_SRC test_aot_decs_modules daslang)

add_custom_target(test_aot_live_host)
SET(LIVE_HOST_AOT_GENERATED_SRC)
DAS_AOT("${AOT_LIVE_HOST_FILES}" LIVE_HOST_AOT_GENERATED_SRC test_aot_live_host daslang)

add_custom_target(test_aot_live_host_modules)
SET(LIVE_HOST_MODULES_AOT_GENERATED_SRC)
DAS_AOT_LIB("${AOT_LIVE_HOST_MODULE_FILES}" LIVE_HOST_MODULES_AOT_GENERATED_SRC test_aot_live_host_modules daslang)

add_custom_target(test_aot_strudel_modules)
SET(STRUDEL_MODULES_AOT_GENERATED_SRC)
IF(NOT DAS_AUDIO_DISABLED)
    DAS_AOT_LIB("${AOT_STRUDEL_MODULE_FILES}" STRUDEL_MODULES_AOT_GENERATED_SRC test_aot_strudel_modules daslang)
ENDIF()

add_custom_target(test_aot_strudel)
SET(STRUDEL_AOT_GENERATED_SRC)
IF(NOT DAS_AUDIO_DISABLED)
    DAS_AOT("${AOT_STRUDEL_FILES}" STRUDEL_AOT_GENERATED_SRC test_aot_strudel daslang)
ENDIF()

add_custom_target(test_aot_pugixml_modules)
SET(PUGIXML_MODULES_AOT_GENERATED_SRC)
IF(NOT DAS_PUGIXML_DISABLED)
    DAS_AOT_LIB("${AOT_PUGIXML_MODULE_FILES}" PUGIXML_MODULES_AOT_GENERATED_SRC test_aot_pugixml_modules daslang)
ENDIF()

add_custom_target(test_aot_pugixml)
SET(PUGIXML_AOT_GENERATED_SRC)
IF(NOT DAS_PUGIXML_DISABLED)
    DAS_AOT("${AOT_PUGIXML_FILES}" PUGIXML_AOT_GENERATED_SRC test_aot_pugixml daslang)
ENDIF()

add_custom_target(test_aot_dashv_modules)
SET(DASHV_MODULES_AOT_GENERATED_SRC)
IF(NOT DAS_HV_DISABLED)
    DAS_AOT_LIB("${AOT_DASHV_MODULE_FILES}" DASHV_MODULES_AOT_GENERATED_SRC test_aot_dashv_modules daslang)
ENDIF()

add_custom_target(test_aot_dashv)
SET(DASHV_AOT_GENERATED_SRC)
IF(NOT DAS_HV_DISABLED)
    DAS_AOT("${AOT_DASHV_FILES}" DASHV_AOT_GENERATED_SRC test_aot_dashv daslang)
ENDIF()

add_custom_target(test_aot_dynamic_cast_rtti)
SET(DYNAMIC_CAST_RTTI_AOT_GENERATED_SRC)
DAS_AOT("${AOT_DYNAMIC_CAST_RTTI_FILES}" DYNAMIC_CAST_RTTI_AOT_GENERATED_SRC test_aot_dynamic_cast_rtti daslang)

add_custom_target(test_aot_fio)
SET(FIO_AOT_GENERATED_SRC)
DAS_AOT("${AOT_FIO_FILES}" FIO_AOT_GENERATED_SRC test_aot_fio daslang)

add_custom_target(test_aot_fs)
SET(FS_AOT_GENERATED_SRC)
DAS_AOT("${AOT_FS_FILES}" FS_AOT_GENERATED_SRC test_aot_fs daslang)

add_custom_target(test_aot_functional)
SET(FUNCTIONAL_AOT_GENERATED_SRC)
DAS_AOT("${AOT_FUNCTIONAL_FILES}" FUNCTIONAL_AOT_GENERATED_SRC test_aot_functional daslang)

add_custom_target(test_aot_handle_types)
SET(HANDLE_TYPES_AOT_GENERATED_SRC)
DAS_AOT("${AOT_HANDLE_TYPES_FILES}" HANDLE_TYPES_AOT_GENERATED_SRC test_aot_handle_types daslang)

add_custom_target(test_aot_hash_map)
SET(HASH_MAP_AOT_GENERATED_SRC)
DAS_AOT("${AOT_HASH_MAP_FILES}" HASH_MAP_AOT_GENERATED_SRC test_aot_hash_map daslang)

add_custom_target(test_aot_interfaces)
SET(INTERFACES_AOT_GENERATED_SRC)
DAS_AOT("${AOT_INTERFACES_FILES}" INTERFACES_AOT_GENERATED_SRC test_aot_interfaces daslang)

add_custom_target(test_aot_jobque)
SET(JOBQUE_AOT_GENERATED_SRC)
DAS_AOT("${AOT_JOBQUE_FILES}" JOBQUE_AOT_GENERATED_SRC test_aot_jobque daslang)

add_custom_target(test_aot_json)
SET(JSON_AOT_GENERATED_SRC)
DAS_AOT("${AOT_JSON_FILES}" JSON_AOT_GENERATED_SRC test_aot_json daslang)

add_custom_target(test_aot_linq)
SET(LINQ_AOT_GENERATED_SRC)
DAS_AOT("${AOT_LINQ_FILES}" LINQ_AOT_GENERATED_SRC test_aot_linq daslang)

add_custom_target(test_aot_linq_modules)
SET(LINQ_MODULES_AOT_GENERATED_SRC)
DAS_AOT_LIB("${AOT_LINQ_MODULE_FILES}" LINQ_MODULES_AOT_GENERATED_SRC test_aot_linq_modules daslang)

add_custom_target(test_aot_match)
SET(MATCH_AOT_GENERATED_SRC)
DAS_AOT("${AOT_MATCH_FILES}" MATCH_AOT_GENERATED_SRC test_aot_match daslang)

add_custom_target(test_aot_math)
SET(MATH_AOT_GENERATED_SRC)
DAS_AOT("${AOT_MATH_FILES}" MATH_AOT_GENERATED_SRC test_aot_math daslang)

add_custom_target(test_aot_math_modules)
SET(MATH_MODULES_AOT_GENERATED_SRC)
DAS_AOT_LIB("${AOT_MATH_MODULE_FILES}" MATH_MODULES_AOT_GENERATED_SRC test_aot_math_modules daslang)

add_custom_target(test_aot_module_tests)
SET(MODULE_TESTS_AOT_GENERATED_SRC)
DAS_AOT("${AOT_MODULE_TESTS_FILES}" MODULE_TESTS_AOT_GENERATED_SRC test_aot_module_tests daslang)

add_custom_target(test_aot_regex)
SET(REGEX_AOT_GENERATED_SRC)
DAS_AOT("${AOT_REGEX_FILES}" REGEX_AOT_GENERATED_SRC test_aot_regex daslang)

add_custom_target(test_aot_safe_addr)
SET(SAFE_ADDR_AOT_GENERATED_SRC)
DAS_AOT("${AOT_SAFE_ADDR_FILES}" SAFE_ADDR_AOT_GENERATED_SRC test_aot_safe_addr daslang)

add_custom_target(test_aot_delegate)
SET(DELEGATE_AOT_GENERATED_SRC)
DAS_AOT("${AOT_DELEGATE_FILES}" DELEGATE_AOT_GENERATED_SRC test_aot_delegate daslang)

add_custom_target(test_aot_soa)
SET(SOA_AOT_GENERATED_SRC)
DAS_AOT("${AOT_SOA_FILES}" SOA_AOT_GENERATED_SRC test_aot_soa daslang)

add_custom_target(test_aot_spoof)
SET(SPOOF_AOT_GENERATED_SRC)
DAS_AOT("${AOT_SPOOF_FILES}" SPOOF_AOT_GENERATED_SRC test_aot_spoof daslang)

add_custom_target(test_aot_strings)
SET(STRINGS_AOT_GENERATED_SRC)
DAS_AOT("${AOT_STRINGS_FILES}" STRINGS_AOT_GENERATED_SRC test_aot_strings daslang)

add_custom_target(test_aot_template)
SET(TEMPLATE_AOT_GENERATED_SRC)
DAS_AOT("${AOT_TEMPLATE_FILES}" TEMPLATE_AOT_GENERATED_SRC test_aot_template daslang)

add_custom_target(test_aot_type_traits)
SET(TYPE_TRAITS_AOT_GENERATED_SRC)
DAS_AOT("${AOT_TYPE_TRAITS_FILES}" TYPE_TRAITS_AOT_GENERATED_SRC test_aot_type_traits daslang)

add_custom_target(test_aot_uri)
SET(URI_AOT_GENERATED_SRC)
DAS_AOT("${AOT_URI_FILES}" URI_AOT_GENERATED_SRC test_aot_uri daslang)

add_custom_target(test_aot_lpipe)
SET(LPIPE_AOT_GENERATED_SRC)
DAS_AOT("${AOT_LPIPE_FILES}" LPIPE_AOT_GENERATED_SRC test_aot_lpipe daslang)

add_custom_target(test_aot_jit)
SET(JIT_AOT_GENERATED_SRC)
DAS_AOT("${AOT_JIT_FILES}" JIT_AOT_GENERATED_SRC test_aot_jit daslang)

add_custom_target(test_aot_language)
SET(LANGUAGE_AOT_GENERATED_SRC)
DAS_AOT("${AOT_LANGUAGE_TEST_FILES}" LANGUAGE_AOT_GENERATED_SRC test_aot_language daslang)

add_custom_target(test_aot_language_modules)
SET(LANGUAGE_MODULES_AOT_GENERATED_SRC)
DAS_AOT_LIB("${AOT_LANGUAGE_MODULE_FILES}" LANGUAGE_MODULES_AOT_GENERATED_SRC test_aot_language_modules daslang)

add_custom_target(test_aot_stbimage)
SET(STBIMAGE_AOT_GENERATED_SRC)
DAS_AOT("${AOT_STBIMAGE_FILES}" STBIMAGE_AOT_GENERATED_SRC test_aot_stbimage daslang)

add_custom_target(test_aot_stbimage_modules)
SET(STBIMAGE_MODULES_AOT_GENERATED_SRC)
DAS_AOT_LIB("${AOT_STBIMAGE_MODULE_FILES}" STBIMAGE_MODULES_AOT_GENERATED_SRC test_aot_stbimage_modules daslang)

add_custom_target(test_aot_tests)
SET(TEST_AOT_GENERATED_SRC)
DAS_AOT("${AOT_TEST_FILES}" TEST_AOT_GENERATED_SRC test_aot_tests daslang)

SOURCE_GROUP_FILES("aot generated" TEST_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" TESTING_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" ALGORITHM_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" APPLY_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" ARCHIVE_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" AST_MATCH_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" ASSERT_ONCE_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" ASYNC_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" BARE_BLOCK_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" BASE64_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" BITFIELDS_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" BOOL_ARRAY_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" DATA_WALKER_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" DEBUG_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" DEBUG_AGENT_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" DECS_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" LIVE_HOST_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" LIVE_HOST_MODULES_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" DYNAMIC_CAST_RTTI_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" FIO_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" FS_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" FUNCTIONAL_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" HANDLE_TYPES_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" HASH_MAP_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" INTERFACES_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" JOBQUE_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" JSON_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" LINQ_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" LINQ_MODULES_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" MATCH_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" MATH_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" MATH_MODULES_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" MODULE_TESTS_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" REGEX_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" SAFE_ADDR_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" DELEGATE_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" SOA_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" SPOOF_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" STRINGS_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" TEMPLATE_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" TYPE_TRAITS_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" URI_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" LPIPE_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" JIT_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" LANGUAGE_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" LANGUAGE_MODULES_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" DASLIB_MODULES_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" DECS_MODULES_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" STBIMAGE_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" STBIMAGE_MODULES_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" STRUDEL_MODULES_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" PUGIXML_MODULES_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" PUGIXML_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" DASHV_MODULES_AOT_GENERATED_SRC)
SOURCE_GROUP_FILES("aot generated" DASHV_AOT_GENERATED_SRC)

# Build the test_aot executable using the same main.cpp as daslang
add_executable(test_aot ${DAS_DASCRIPT_MAIN_SRC}
    ${TEST_AOT_GENERATED_SRC}
    ${TESTING_AOT_GENERATED_SRC}
    ${ALGORITHM_AOT_GENERATED_SRC}
    ${APPLY_AOT_GENERATED_SRC}
    ${ARCHIVE_AOT_GENERATED_SRC}
    ${AST_MATCH_AOT_GENERATED_SRC}
    ${ASSERT_ONCE_AOT_GENERATED_SRC}
    ${ASYNC_AOT_GENERATED_SRC}
    ${BARE_BLOCK_AOT_GENERATED_SRC}
    ${BASE64_AOT_GENERATED_SRC}
    ${BITFIELDS_AOT_GENERATED_SRC}
    ${BOOL_ARRAY_AOT_GENERATED_SRC}
    ${DATA_WALKER_AOT_GENERATED_SRC}
    ${DEBUG_AOT_GENERATED_SRC}
    ${DEBUG_AGENT_AOT_GENERATED_SRC}
    ${DECS_AOT_GENERATED_SRC}
    ${LIVE_HOST_AOT_GENERATED_SRC}
    ${LIVE_HOST_MODULES_AOT_GENERATED_SRC}
    ${DYNAMIC_CAST_RTTI_AOT_GENERATED_SRC}
    ${FIO_AOT_GENERATED_SRC}
    ${FS_AOT_GENERATED_SRC}
    ${FUNCTIONAL_AOT_GENERATED_SRC}
    ${HANDLE_TYPES_AOT_GENERATED_SRC}
    ${HASH_MAP_AOT_GENERATED_SRC}
    ${INTERFACES_AOT_GENERATED_SRC}
    ${JOBQUE_AOT_GENERATED_SRC}
    ${JSON_AOT_GENERATED_SRC}
    ${LINQ_AOT_GENERATED_SRC}
    ${LINQ_MODULES_AOT_GENERATED_SRC}
    ${MATCH_AOT_GENERATED_SRC}
    ${MATH_AOT_GENERATED_SRC}
    ${MATH_MODULES_AOT_GENERATED_SRC}
    ${MODULE_TESTS_AOT_GENERATED_SRC}
    ${REGEX_AOT_GENERATED_SRC}
    ${SAFE_ADDR_AOT_GENERATED_SRC}
    ${DELEGATE_AOT_GENERATED_SRC}
    ${SOA_AOT_GENERATED_SRC}
    ${SPOOF_AOT_GENERATED_SRC}
    ${STRINGS_AOT_GENERATED_SRC}
    ${TEMPLATE_AOT_GENERATED_SRC}
    ${TYPE_TRAITS_AOT_GENERATED_SRC}
    ${URI_AOT_GENERATED_SRC}
    ${LPIPE_AOT_GENERATED_SRC}
    ${JIT_AOT_GENERATED_SRC}
    ${LANGUAGE_AOT_GENERATED_SRC}
    ${LANGUAGE_MODULES_AOT_GENERATED_SRC}
    ${DASLIB_MODULES_AOT_GENERATED_SRC}
    ${DECS_MODULES_AOT_GENERATED_SRC}
    ${STBIMAGE_AOT_GENERATED_SRC}
    ${STBIMAGE_MODULES_AOT_GENERATED_SRC}
    ${STRUDEL_MODULES_AOT_GENERATED_SRC}
    ${STRUDEL_AOT_GENERATED_SRC}
    ${PUGIXML_MODULES_AOT_GENERATED_SRC}
    ${PUGIXML_AOT_GENERATED_SRC}
    ${DASHV_MODULES_AOT_GENERATED_SRC}
    ${DASHV_AOT_GENERATED_SRC}
)
TARGET_LINK_LIBRARIES(test_aot libDaScriptAot ${SRC_LIBRARIES} ${DAS_MODULES_LIBS})
ADD_DEPENDENCIES(test_aot libDaScriptAot
    test_aot_tests test_aot_testing
    test_aot_algorithm test_aot_apply test_aot_archive test_aot_ast_match
    test_aot_assert_once test_aot_async test_aot_bare_block test_aot_base64
    test_aot_bitfields test_aot_bool_array
    test_aot_data_walker test_aot_debug
    test_aot_debug_agent test_aot_decs test_aot_live_host test_aot_live_host_modules test_aot_dynamic_cast_rtti
    test_aot_fio test_aot_fs test_aot_functional
    test_aot_handle_types test_aot_hash_map
    test_aot_interfaces
    test_aot_jobque test_aot_json
    test_aot_linq test_aot_linq_modules
    test_aot_match
    test_aot_math test_aot_math_modules test_aot_module_tests
    test_aot_regex
    test_aot_delegate test_aot_safe_addr test_aot_soa test_aot_spoof test_aot_strings
    test_aot_template test_aot_type_traits test_aot_uri
    test_aot_lpipe
    test_aot_jit
    test_aot_language test_aot_language_modules
    test_aot_daslib_modules test_aot_decs_modules
    test_aot_stbimage test_aot_stbimage_modules
    test_aot_strudel
    test_aot_pugixml test_aot_pugixml_modules
    test_aot_dashv test_aot_dashv_modules
)
target_include_directories(test_aot PRIVATE ${NEED_MODULES_PATH} ${PROJECT_SOURCE_DIR}/modules/dasUnitTest ${PROJECT_SOURCE_DIR}/modules/dasLiveHost/src ${AUDIO_INCLUDE_DIR} ${CIPIC_HRTF_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/modules/dasMinfft/minfft ${PROJECT_SOURCE_DIR}/modules/dasPUGIXML/pugixml ${PROJECT_SOURCE_DIR}/modules/dasHV/hv/include)
target_compile_definitions(test_aot PRIVATE DAS_TEST_AOT MINFFT_SINGLE HRTF_SAMPLE_RATE=${DAS_AUDIO_SAMPLE_RATE})
SETUP_CPP11(test_aot)
SETUP_LTO(test_aot)
