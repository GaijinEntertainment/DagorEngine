# Test Suite Index

> **Keep up to date:** When adding, removing, or renaming test files under `tests/`, update this file. See `skills/writing_tests.md` for the rule.

Every `.das` file in this directory tree is listed below, grouped by subdirectory. Files marked **expect** use `expect` directives and are expected to produce specific compile errors. Helper/module files that are not standalone tests are marked *(helper)*.

## live_host/

> **Note:** These tests require the dasLiveHost module. Skipped automatically via `.das_test` when the module is not available. Run separately: `dastest -- --test tests/live_host/`

| File | Description | Expects errors |
|---|---|---|
| test_store.das | Persistent byte store — store/load round-trip, overwrite, missing keys, empty data, large data, multiple keys | |
| test_lifecycle.das | Lifecycle functions — get_dt, get_uptime, get_fps, is_paused/set_paused, exit_requested, is_reload, is_live_mode | |
| test_commands.das | Live command dispatch — [live_command] registration, JSON dispatch, args passing, null return, error handling | |
| test_live_vars.das | `@live` variable viewer — annotation, archive round-trip, type coverage (int, float, string, bool) | |

## algorithm/

| File | Description | Expects errors |
|---|---|---|
| sets.das | Set operations — intersection, union, difference | |
| test_algorithm.das | unique, sort, erase, find, and other core algorithms | |
| test_algorithm_new.das | upper_bound, lower_bound, custom comparators | |
| test_algorithm_noncopyable.das | Algorithm operations on non-copyable structs (erase_all, sort, swap) | |
| topological_sort.das | Topological sort on dependency graphs | |

## aot/

| File | Description | Expects errors |
|---|---|---|
| test_aot_assert.das | AOT assert behavior — assert pass/fail, verify, static_assert | |
| test_arithmetic.das | AOT arithmetic — basic ops, type promotion, AOT active check | |
| test_lambdas.das | AOT lambda codegen — capture, invoke, nested | |
| test_strings.das | AOT string operations — interpolation, comparison, builder | |
| test_structures.das | AOT struct codegen — construction, fields, methods, inheritance | |

## apply/

| File | Description | Expects errors |
|---|---|---|
| test_apply.das | `apply` macro on variants, structs, nested types | |

## archive/

| File | Description | Expects errors |
|---|---|---|
| test_archive.das | mem_archive_save/load for variants, tables, arrays, structs | |

## assert_once/

| File | Description | Expects errors |
|---|---|---|
| test_assert_once.das | `assert_once` fires only on first iteration (loop + try/recover) | |

## async/

| File | Description | Expects errors |
|---|---|---|
| test_async_await.das | let/var await, copy/move/clone assign await, void await, nested await, await in loops | |
| test_async_basic.das | `[async]` annotation, void/typed async, await_next_frame, yield, early return | |
| test_async_coroutine.das | async/coroutine interop — mixing `[async]` with `[coroutine]`, awaiting coroutines | |
| test_async_iter.das | Iterating async generators — for-loop over typed async, zip with count() | |
| test_async_run.das | async_run, async_run_all, async_timeout, async_race | |

## base64/

| File | Description | Expects errors |
|---|---|---|
| test_base64.das | base64 encode/decode with fuzzing | |

## bitfields/

| File | Description | Expects errors |
|---|---|---|
| test_bitfield_boost.das | bitfield_boost — indexing, each_bit, &&= operator | |

## bool_array/

| File | Description | Expects errors |
|---|---|---|
| test_bool_array.das | BoolArray — push, index, clear, resize, iteration, operators | |
| test_bool_array_iterator_crash.das | BoolArray iterator context mismatch repro — benchmark context isolation | |

## bare_block/

| File | Description | Expects errors |
|---|---|---|
| test_assumed_pipe.das | Assumed pipe — block invocation via `<\|` with `assume` aliases | |
| test_bare_block.das | Bare lexical blocks — scoping, nesting, finally, control flow, name reuse | |
| test_nested_data.das | Table `{}` literals as sub-data — structs, arrays, functions, tuples, variants, comprehensions, move semantics | |
| _test_table_compat.das | Basic table literal compatibility tests | |

## constexpr/

| File | Description | Expects errors |
|---|---|---|
| test_constexpr_fail.das | `[constexpr]` annotation rejects non-constant arguments | **expect** `40102:1` |

## data_walker/

| File | Description | Expects errors |
|---|---|---|
| dw_common.das | *(helper)* Shared module — LogWalker class and test data types | |
| test_walk_containers.das | DapiDataWalker on arrays, dim, tables | |
| test_walk_edge_cases.das | Null/non-null pointers, lambdas, edge values, deeply nested data | |
| test_walk_filtering.das | canVisitStructure, canVisitArray, canVisitArrayData filtering | |
| test_walk_mutation.das | In-place mutation via walker callbacks | |
| test_walk_scalars.das | Walking scalar types — int, uint, float, double, bool, string, enum | |
| test_walk_structs.das | Walking structures, classes, nested structs, field-last flags | |
| test_walk_tuples_variants.das | Walking tuples and variants | |
| test_walk_vectors_ranges.das | Walking int2/3/4, float2/3/4, range, urange, range64 | |

## dasHV/

> **Note:** These tests require the dasHV module (`DAS_HV_DISABLED=OFF`). Run: `dastest -- --test tests/dasHV/`

| File | Description | Expects errors |
|---|---|---|
| _dashv_test_common.das | *(helper)* Shared module — port constants, `with_test_server` lifecycle, file helpers | |
| test_client_basic.das | Fire-and-forget client API — GET, POST, PUT, PATCH, HTTP_DELETE, HEAD, headers, status_message, get_body_bytes | |
| test_request_builder.das | Request builder — with_http_request, request(), set_header, timeouts, auth, params, redirect | |
| test_server_routes.das | Server routing — GET/POST/PUT/DEL/PATCH/HEAD/ANY routes, path params, query params, JSON | |
| test_server_advanced.das | Advanced server — STATIC, CORS, REDIRECT, DATA, custom content-type, status codes (201, 204, 404) | |
| test_cookies.das | Cookie operations — add_cookie (simple + extended, req + resp), get_cookie, each_cookie | |
| test_forms.das | Form data — set_form_data, set_form_file, get_form_data, save_form_file, each_form_field, URL-encoded | |
| test_sse.das | SSE (Server-Sent Events) — server-side SSE handler, client-side streaming via request_cb | |
| test_websockets.das | WebSocket client/server — connect, send, receive, broadcast, multiple clients, HTTP+WS coexistence | |

## dasPUGIXML/

> **Note:** These tests require the dasPUGIXML module (`DAS_PUGIXML_DISABLED=OFF`). Run: `dastest -- --test tests/dasPUGIXML/`

| File | Description | Expects errors |
|---|---|---|
| test_pugixml_boost.das | PUGIXML_boost helpers — iteration (for_each, each, each_child, each_attribute), RAII, accessors, builders, serialization, operator is/as type conversions | |
| test_pugixml_core.das | Core C++ bindings — node navigation, attribute/text properties, parse result, document ops, XPath evaluation | |
| test_pugixml_mutation.das | XML mutation — node/attribute insert/remove/copy/move, text set, attribute set_value types | |
| test_serial_struct.das | Struct serialization — XML()/from_XML()/to_XML() for simple, nested, and array-containing structs | |
| test_serial_tuple.das | Tuple serialization — unnamed and named tuples, structs with tuple fields | |
| test_serial_variant.das | Variant serialization — int/string variant cases, struct with variant field, _variant attribute | |
| test_serial_dim.das | Fixed array (dim) serialization — int[N], string[N], struct with dim field | |
| test_serial_table.das | Table and set serialization — table<string;int>, table<string>, key-only sets, empty tables | |
| test_serial_enum.das | Enum serialization — string-based enum roundtrip, default values | |
| test_serial_bitfield.das | Bitfield serialization — Perms (uint), Flags8 (uint8), Flags16 (uint16), Flags64 (uint64), high-bit test | |
| test_serial_integers.das | Integer type serialization — int8, uint8, int16, uint16, int64, uint64, standalone and struct fields | |
| test_serial_vectors.das | Vector/range serialization — float2/3/4, int2/3/4, uint4, range, urange, range64, urange64 | |
| test_serial_annotations.das | Annotation support — @rename, @enum_as_int, mixed annotations, roundtrip with annotations | |

## daslib/

| File | Description | Expects errors |
|---|---|---|
| ast_cursor_fixture.das | *(helper)* Fixture file for ast_cursor tests | |
| ast_cursor_test.das | `daslib/ast_cursor` — cursor-based AST navigation and query | |

## dastest/

| File | Description | Expects errors |
|---|---|---|
| _fixture_bench.das | *(helper)* Benchmark fixture for dastest framework tests | |
| _fixture_fail.das | *(helper)* Failing test fixture | |
| _fixture_pass.das | *(helper)* Passing test fixture | |
| _fixture_skip.das | *(helper)* Skipped test fixture | |
| test_json_output.das | dastest JSON output — `--json-file` report format, suite results, pass/fail/skip counts | |

## debug/

| File | Description | Expects errors |
|---|---|---|
| deval.das | debug_eval — expression evaluation in debug context | |
| test_sprint_format.das | sprint/sprint_data gen2 output format for all types and fullTypeInfo flag | |

## debug_agent/

| File | Description | Expects errors |
|---|---|---|
| test_invoke_in_context.das | invoke_in_context with `[pinvoke]` — cross-context function invocation | |
| test_invoke_method.das | invoke_debug_agent_method with various argument counts | |
| test_lifecycle.das | Debug agent lifecycle — create, check, context access | |
| test_on_log.das | to_log/print triggering onLog on debug agents | |
| test_state_collection.das | State collection via onCollect and onVariable | |
| test_threadlock.das | threadlock_context option propagation | |

## decs/

| File | Description | Expects errors |
|---|---|---|
| dim_test.das | decs_template with fixed-size array (dim) components | |
| empty_query.das | Empty decs query and stage function | |
| failed_query_eid_ro.das | Read-only query on const component fails | **expect** `30303` |
| failed_test_arguments.das | Invalid query arguments (optional with defaults, & + const) | **expect** `40104:4` `30103:2` |
| failed_test_empty_template.das | Empty decs_template struct is rejected | **expect** `30111` |
| test_archetype.das | Archetype creation, lookup, migration | |
| test_bulk_create.das | Bulk entity creation — `create_entities` and `create_entities`T` direct path | |
| test_arrays.das | Arrays as decs components — iterate, modify | |
| test_boundaries.das | commit/restart inside query (expected failures) | |
| test_commit_deferred.das | Deferred semantics — entities not visible until commit | |
| test_component_map.das | ComponentMap set/get/has/remove | |
| test_create_update_remove.das | Entity create, update, remove lifecycle | |
| test_default_values.das | decs_template with default values | |
| test_entity_lifecycle.das | Entity ID uniqueness, validity, invalidation after delete | |
| test_gc.das | decs with GC — archive arrays, strings, gc_collect | |
| test_objects.das | Creating objects/entities with set and clone syntax | |
| test_optional_values.das | `@optional` query arguments | |
| test_queries.das | Nested queries, by-eid, with different types | |
| test_queries_comprehensive.das | Query across archetypes, mutation, const-correctness | |
| test_query_eid_rw.das | Read-write query with eid parameter | |
| test_serialize.das | decs serialization — save/load state | |
| test_stages.das | decs_stage — update stage with velocity | |
| test_stages_comprehensive.das | Physics/scoring stages, REQUIRE filtering | |
| test_stages_extra.das | *(helper)* Extra stage functions — make_invisible, check_visible | |
| test_templates_comprehensive.das | decs_template — default prefix, custom prefix, nested templates | |
| test_update_entity_comprehensive.das | update_entity — modify, add/remove components, archetype migration | |
| test_utility_functions.das | is_alive, INVALID_ENTITY_ID, entity counting | |

## dynamic_cast_rtti/

| File | Description | Expects errors |
|---|---|---|
| instance_and_cast.das | is_instance_of and dynamic_cast_rtti | |

## fio/

| File | Description | Expects errors |
|---|---|---|
| fio_errors.das | Path manipulation and error handling — extension, dir_name, base_name, normalize, mkdir/rmdir edge cases | |
| fio_file.das | File I/O — fopen, fread, fwrite with fuzzing | |
| fio_utils.das | File utilities — fexist, rmdir, rmdir_rec, fread/fwrite by path, get_das_version | |

## functional/

| File | Description | Expects errors |
|---|---|---|
| test_functional.das | flatten on nested iterators | |
| test_functional_edge.das | Edge cases — map, filter, reduce, zip, scan, take_while, skip_while, etc. | |

## fs/

| File | Description | Expects errors |
|---|---|---|
| test_fs.das | Filesystem operations — extension, dir_name, base_name, normalize, scan_dir, stat, mkdir/rmdir | |

## handle_types/

| File | Description | Expects errors |
|---|---|---|
| handle_vector.das | Point3Array handle type — indexing, each, custom operator [] | |

## hash_map/

| File | Description | Expects errors |
|---|---|---|
| all_hash_table.das | flat_hash_table and cuckoo_hash_table — dispatch, generics | |

## delegate/

| File | Description | Expects errors |
|---|---|---|
| test_delegate.das | C#-like delegate system — assign, append, invoke, void/non-void, multi-arg, zero-arg, utilities | |

## interfaces/

| File | Description | Expects errors |
|---|---|---|
| test_const_interfaces.das | Const-only interfaces — all-const methods, mixed const/non-const, const dispatch | |
| test_inheritance.das | Interface inheritance — IAnimal → IPet → IGuideDog | |
| test_interfaces.das | `[interface]` macro — ITick, ILogger, IEmpty, `[implements]` | |
| test_missing_inherited.das | Completeness check catches missing inherited abstract methods | **expect** `30111` |
| test_missing_method.das | Completeness check catches missing abstract methods | **expect** `30111` |

## jit_tests/

48 files testing JIT compilation code generation. None have `expect` directives.

| File | Description | Expects errors |
|---|---|---|
| addr.das | Address operations | |
| arithmetics.das | Arithmetic operations with fuzzing | |
| array.das | Array operations | |
| assert.das | Assert behavior | |
| bit_arithmetics.das | Bitwise arithmetic | |
| bitfield.das | Bitfield operations | |
| block.das | Block invocation | |
| bool_arithmetics.das | Boolean arithmetic | |
| chain_invoke_method.das | Chained method invocation | |
| cmres.das | CMRes (call-me-result) return convention | |
| compare.das | Comparison operators | |
| dim.das | Fixed-size arrays (dim) | |
| dim2d.das | 2D fixed arrays | |
| extern_call.das | External function calls | |
| finally.das | Finally blocks | |
| for_loop.das | For loop codegen | |
| generator.das | Generator codegen | |
| global_variables.das | Global variable access | |
| global_variables_solid.das | Solid global variables | |
| handle_types.das | Handle type operations | |
| interop_call.das | Interop calls | |
| intrinsics.das | Intrinsic functions | |
| invoke.das | Invoke operations | |
| jit_exe.das | JIT executable creation — compile and run standalone JIT binary | |
| labels.das | Label/goto codegen | |
| memset.das | Memset operations | |
| move.das | Move semantics | |
| new_ascend_and_delete.das | New/delete heap operations | |
| phi.das | Phi node (SSA) codegen | |
| pointer.das | Pointer operations | |
| ptr_ref_return.das | Pointer ref return — returning `int? const&` from array | |
| range64.das | 64-bit range operations | |
| ref.das | Reference semantics | |
| safe_navigation.das | Safe navigation (?.) | |
| string_builder.das | String builder operations | |
| string_op.das | String operations | |
| struct.das | Struct operations | |
| table.das | Table operations | |
| table_value_key.das | Table value/key access | |
| try_recover.das | Try/recover codegen | |
| tuple.das | Tuple operations | |
| type_constructors.das | Type constructor codegen | |
| typeinfo.das | Typeinfo intrinsics | |
| using_and_placement_new.das | Using/placement new | |
| variant.das | Variant operations | |
| vector_navigation.das | Vector field navigation | |
| vector_swizzle.das | Vector swizzle operations | |
| while_loop.das | While loop codegen | |

## jobque/

| File | Description | Expects errors |
|---|---|---|
| atomics.das | Atomic32/atomic64 set/get/inc/dec | |
| test_jobque_atomics.das | with_atomic32/atomic64 — set, get, inc, dec, initial value | |
| test_jobque_channels.das | Channel boost — push_clone, for_each_clone, with_channel | |
| test_jobque_edge.das | Channel edge cases — single-item, large batch | |
| test_jobque_jobs.das | JobStatus, new_job with channels and messages | |
| test_jobque_lockbox.das | LockBox basics — create, destroy, validity | |
| test_jobque_parallel.das | parallel_for, parallel work distribution | |
| test_jobque_try_pop.das | try_pop_clone — non-blocking pop, empty/data/drain | |
| test_jobque_wait_group.das | with_wait_group — convenience wrapper for job completion | |

## json/

| File | Description | Expects errors |
|---|---|---|
| broken.das | Parsing/round-tripping JSON with emoji and long strings | |
| safe.das | Safe JSON operators — ?[], ?., ?? fallback | |
| test_json_edge.das | JSON edge cases — parsing scalars, arrays, nested, whitespace, error recovery | |
| test_sprint_json.das | sprint_json — basic types, structs, arrays, tables, variants, annotations, RTTI | |
| test_sscan_json.das | sscan_json — scalars, structs, pointers, arrays, tables, tuples, variants, vectors, ranges, bitfields, enums, @rename, round-trip | |
| types.das | JV/from_JV JSON serialization for all basic/vector/struct types | |

## language/

| File | Description | Expects errors |
|---|---|---|
| _glob.das | *(helper)* Shared module defining `AAA = 10` | |
| _helper_foo.das | *(helper)* Module providing `TestObjectFoo` struct and `testFoo` function | |
| _module_a.das | *(helper)* Module for module_vis_fail — globals, types, functions | |
| _module_b.das | *(helper)* Module for module_vis_fail — requires _module_a | |
| _operators_derived.das | *(helper)* Derived class BarOp | |
| _operators_parent.das | *(helper)* Parent class FooOp with property operator | |
| access_private_from_lambda.das | Class private field access from lambda + operator delete | |
| add_property_ext_const.das | `isReadOnly` property on `TestObjectFoo` — const vs non-const dispatch | |
| aka.das | `aka` variable aliasing in various contexts | |
| aliasing.das | Aliasing behavior with `no_aliasing = false` | |
| array.das | Dynamic array ops — make, verify, pass, erase, push, sort, move, multi-push, const | |
| array_comprehension.das | Array/iterator comprehension — square, filtered with `where` | |
| ascend_ctor.das | `new` with class constructors and fields | |
| assume.das | `assume` for deep field access | |
| failed_assume_loop.das | Circular `assume` detection — direct and indirect loops | **expect** `30127:2` `30305:4` |
| failed_auto_infer.das | Auto type inference failures — missing overloads, wrong types | **expect** `31101` `31102:8` `30304:2` `30105:1` `30102:2` `30106` `30113` `31300` |
| auto_infer_success.das | Successful auto type inference — generics, dim, static_if, variants | |
| auto_ref_and_move_ret.das | Auto ref alias and move return of array | |
| bin_serializer.das | `binary_save`/`binary_load` — struct with bool, int, float, array, fixed_array fields | |
| bitfields.das | Bitfield operations — 32-bit and 64-bit, operators, `typeinfo` | |
| block.das | Block creation and invocation | |
| block_invoke.das | Block invocation — twice, nested chaining, value capture, ref passing | |
| block_access_function_arg.das | Nested block accessing outer lambda variable via helper | |
| block_args_nested.das | Deeply nested blocks — int, ref, ptr, struct passthrough | |
| block_variable.das | Local block variables — void/result × no-arg/with-arg × value/cmres | |
| block_vs_local_block.das | Pipe `<\|` block vs local block variable invoke | |
| bool_condition.das | Boolean condition in `if` — false skips body | |
| cant_access_private_members.das | Private member access violations | **expect** `30503:3` `30301:1` |
| cant_dereference_mix.das | Invalid dereference operations | **expect** `30501:5` |
| cant_derive_from_sealed_class.das | Sealed class derivation error | **expect** `30115` |
| cant_get_field.das | Invalid field access on types | **expect** `30503:4` `30504` |
| cant_have_local_variable.das | Non-local type as local variable (uses `ast::TypeDecl`) | **expect** `30108:4` `30101:2` `31300:3` |
| cant_index.das | Invalid index operations | **expect** `30502:2` |
| cant_override_sealed.das | Sealed method override errors | **expect** `30115:2` |
| cant_write_to_constant_value.das | Const value write violations | **expect** `30504:3` |
| capture_as_ref.das | Class lambda capturing `self` as ref | |
| capture_string.das | String capture in lambdas and string builder (module) | |
| cast.das | `cast<>` struct inheritance, `reinterpret<>` memory reinterpretation | |
| check_eid.das | `CheckEid`/`CheckEid2` on `TestObjectFoo` with custom `unit_test` option | |
| clone.das | Clone complex nested structs, arrays, tables — deep copy verification | |
| clone_temp.das | Clone of temporary struct returned from function | |
| clone_to_move.das | Clone-assign `:=` for arrays — global and local | |
| copy_and_move_on_return.das | Copy on return + move on return + block return | |
| ctor.das | Struct default initializer and custom constructor | |
| chain_invoke_method.das | Chained `new Obj()->method1()->method2()` invocation | |
| failed_comment_eof.das | Unterminated comment at end of file | **expect** `10007` |
| failed_condition_must_be_bool.das | Non-bool condition in if/while | **expect** `30601:2` `30303` `30506` |
| failed_const_and_block_folding.das | Constant folding side-effect errors | **expect** `40209:2` |
| failed_const_ref.das | Const ref restrictions — write, move, copy, return | **expect** `30102` `30106` `30303:1` `30304:2` `30504:5` `30505:2` `30507:3` `30508:1` |
| containers.das | Temporary container operations (with containers.inc) | |
| failed_containers_failed.das | Container compile-time failures | **expect** `30304:4` |
| contracts.das | `daslib/contracts` — expect_dim, expect_any_tuple, expect_any_variant | |
| coroutines.das | Typed coroutines — count_up, count_down, yield, co_continue | |
| cpp_layout.das | `[cpp_layout]` struct annotation — sizeof, offsetof, pod=false | |
| das_string.das | `das_string` type — assign, peek, string conversion, interpolation | |
| defer.das | Defer execution order, defer_delete, short pipe syntax | |
| default_method_arguments.das | Default argument values in abstract/override methods | |
| deref_ptr_fun.das | `deref(getPtr())` — function pointer dereference | |
| dim.das | Fixed-size 2D arrays — indexing, pointer arithmetic, string representation | |
| div_by_zero.das | Division/modulo by zero — int, uint, int64, uint64 via try/recover | |
| dummy.das | `SomeDummyType` — sizeof, alignof with `@safe_when_uninitialized` | |
| failed_duplicate_keys.das | Duplicate table keys at compile time | **expect** `40300:4` |
| dynamic_array.das | Dynamic array — push, resize, reserve, capacity, erase, move | |
| dynamic_type_checking.das | dynamic_cast, is_instance_of with class hierarchies | |
| each_std_vector.das | `testFooArray()` iteration over std::vector — length, capacity, find_index_if | |
| enum.das | Enum ops — int16 base, negative values, typeinfo, enum_trait, C++ bindings | |
| enumerations.das | Enum conversion — to/from string, enum_trait | |
| erase_if.das | `erase_if` on dynamic arrays — remove negative values, stress patterns | |
| failed_aka.das | Global aka and typedef aka produce errors | **expect** `20000:1` |
| failed_aliasing.das | Aliasing errors with `no_aliasing` option | **expect** `40211:23` `40212:3` |
| failed_block.das | Block variable initialization failures | **expect** `30108` `30113` |
| failed_call_depth.das | Recursive default argument expansion — max call depth exceeded | **expect** `41000:1` `30301:50` |
| failed_capture_self.das | Capturing `self` in lambda fails | **expect** `30508` `30124` |
| failed_constants.das | Out-of-range numeric literal errors | **expect** `10006:12` `10010:4` |
| failed_if_one_liner_multiline.das | One-liner if/else/elif body on separate line is an error | **expect** `20000:7` |
| failed_table_lookup_collision.das | Table lookup collision lint — same table indexed twice in one expression | **expect** `40216:7` |
| finally.das | `finally` blocks — exceptions, loops, nested, return | |
| for_const_array.das | For-loop over `fixed_array` constant | |
| for_continue.das | `continue` in while, for, and complex nested loops | |
| for_loop.das | For-loop mechanics — range, collections, nested, iterators | |
| for_single_element.das | For-loop over single-element dim array | |
| fully_qualified_generic_name.das | Fully qualified `UnitTest::start_effect` generic call | |
| func_addr.das | Function pointer via `@@` — invoke, comparison, null check | |
| if_one_liner.das | One-liner `if` — postfix, prefix, break, continue on same line | |
| if_not_null.das | `if_not_null` macro — null skips, non-null invokes block | |
| ignore_deref.das | Pointer deref from array — by value and explicit type | |
| index_types.das | Index `[]` operations across all types and index types — pointer, array, dim, string | |
| infer_alias_and_alias_ctor.das | Auto alias inference — generic join, findObject | |
| infer_alias_argument.das | Auto alias argument inference — fold | |
| infer_remove_ref_const.das | Auto ref/const removal inference | |
| inscope_return_inscope.das | Early return with `finally` delete of inscope pointers | |
| failed_function_already_declared.das | Duplicate function declaration | **expect** `30201` |
| failed_function_argument_already_declared.das | Duplicate function argument name | **expect** `30202` |
| failed_function_not_found_ambiguous.das | Ambiguous function call — same name in two modules | **expect** `30304` |
| generators.das | Generator mechanics — yield, ranges, nested, early return | |
| failed_global_init_type_mismatch.das | Global variable type mismatch on init | **expect** `30113` |
| global_order.das | Global variable initialization ordering with clone | |
| global_ptr_init.das | Global pointer struct initialization | |
| failed_global_variable_already_declared.das | Duplicate global variable declaration | **expect** `30204` |
| failed_global_variable_init_loop.das | Global variable circular initialization — direct and indirect loops | **expect** `30305:5` |
| failed_global_variable_order.das | Global variable initialization ordering | **expect** `30305:1` |
| failed_global_variable_order_itself.das | Global variable self-initialization error | **expect** `30305:1` |
| handle.das | Handled types — TestObjectFoo/Bar pointer ops, FancyClass, checkRange | |
| hash.das | Hash function for various types | |
| init_order.das | `[init]` function ordering with before/tag | |
| int_types.das | Integer literal types, char literal, small integer operators | |
| intrinsics.das | clz, ctz, popcnt for u32/u64 with fuzzing | |
| invoke_cmres.das | CMRES invoke — functions, hybrid, pointers, blocks, lambdas | |
| invalid_argument_count_mix.das | Wrong argument count in function calls | **expect** `30107:4` |
| invalid_array_type_mix.das | Invalid array element types | **expect** `30105:2` |
| invalid_block.das | Invalid block usage — wrong types, missing args | **expect** `30801` `30304:4` `32102` `32101` `30102` |
| invalid_escape_sequence.das | Invalid string escape sequence | **expect** `10008` |
| invalid_index_type.das | Wrong index type for containers | **expect** `30110:3` `30106` |
| invalid_infer_return_type.das | Return type inference failures | **expect** `30102` `32101` `32102` |
| invalid_return_type_mix.das | Mismatched return types | **expect** `30102` `30102` |
| invalid_structure_field_type_ref.das | Ref type in struct field | **expect** `30104` |
| invalid_structure_field_type_void.das | Void type in struct field | **expect** `30104` |
| invalid_table_type_mix.das | Invalid table key/value types | **expect** `30106:2` `30108` |
| invalid_type_ref_in_table_value.das | Ref type as table value | **expect** `30106` |
| invalid_types.das | Oversized types and arguments | **expect** `30101:2` `30108:4` `30109:2` |
| failed_jit_abi.das | JIT ABI correctness — `test_abi_mad` for float2/3/4, function pointers | |
| labels.das | Labels and goto — control flow, nested loops, labeled break | |
| lambda_basic.das | Lambda capture, invoke, null check, addX returning lambda | |
| lambda_capture.das | Lambda capturing const values, finalizer behavior | |
| lambda_capture_modes.das | Lambda capture ref/move/clone modes, capture with delete | |
| lambda_to_iter.das | Lambda as iterator via `each(lam)` for int& and struct | |
| line_info.das | `testCallLine()` line info correctness (6 in script, 0 with AOT) | |
| failed_local_classes_failed.das | no_local_class_members restriction | **expect** `31300:1` |
| lock_array.das | `lock`/`lock_data` on arrays — read-only iteration, mutable data access | |
| loop_ret.das | Loop early return fills unique entries (random + tuple) | |
| make_default.das | Default values for int, string, tuple, variant, struct, enum, array | |
| make_handle.das | Handle construction — `TestObjectFoo(fooData=...)`, global/local/cmres/ascend | |
| make_local.das | Struct local construction — defaults, `uninitialized`, fixed_array | |
| make_struct_with_clone.das | Struct construction with clone `:=` for array fields | |
| map_to_a.das | `map_to_array` — reinterpret raw memory as typed array via unsafe block | |
| memset.das | memset8, memset16, memset32, memset64, memset128 | |
| memzero.das | memzero for float and fixed_array | |
| method_semantic.das | Struct with `@@` function pointer fields — magnitude, dot product | |
| failed_mismatching_curly_bracers.das | Mismatched `{` `}` brackets | **expect** `20000` `10002` |
| failed_mismatching_parentheses.das | Mismatched `(` `)` parentheses | **expect** `20000` `10001` |
| mksmart_zero.das | `default<smart_ptr<TestObjectSmart>>` returns null smart pointer | |
| failed_module_vis_fail.das | Module visibility and scope errors | **expect** `30304:4` `30305:2` `30301:2` |
| move_and_return_move.das | Move semantics — self-move, function move, struct with arrays | |
| move_lambda_local_ref.das | `capture(<- arr)` moving array into lambda | |
| move_on_return.das | Move with `finally` delete — pipeline array return | |
| failed_named_call.das | Named arguments — reordering, skipping, defaults, error cases | **expect** `30304:12` `30101:1` `30507:1` |
| new_and_init.das | `new_and_init` — allocate and copy struct with `always_export_initializer` | |
| new_delete.das | `delete` for arrays, tables, structs, handles, strings — heap tracking | |
| new_with_init.das | `new` with struct constructors — zeroed, defaults, custom, array | |
| failed_new_type_infer.das | `new` type inference failures | **expect** `30109` `30301` |
| no_default_initializer.das | `[no_default_initializer]` annotation | |
| failed_no_init.das | `options no_init` preventing `[init]` functions | **expect** `40214:3` |
| failed_not_all_paths_return_a_value.das | Missing return in some code paths | **expect** `40200` |
| operator_overload.das | Custom `operator -`, `operator +`, `operator ==` on user struct | |
| operators.das | Custom operators — `as`, `?as`, derived class operators | |
| oop.das | Classes — constructors, finalizers, inheritance, RTTI, virtual dispatch | |
| option_type.das | Option types (int& \| auto) ref preservation | |
| override_field.das | Struct field `override` for function pointers in derived struct | |
| partial_specialization.das | Generic function specialization dispatch | |
| peek_and_modify_string.das | `peek_data` finds chars, `modify_data` replaces bytes | |
| pointers.das | Pointer operations — new, deref, safe navigation, null checks | |
| ptr_arithmetic.das | Pointer arithmetic — signed/unsigned int/int64/uint/uint64 | |
| ptr_index.das | Pointer deref and index, default null pointer argument | |
| properties.das | Property operators (.res :=, getter/setter) | |
| random_numbers.das | Random seeding, distributions, reproducibility | |
| reflection.das | RTTI reflection — compile source, inspect modules/structs/enums/functions | |
| failed_reserved_names.das | Use of reserved identifier names | **expect** `30116:9` |
| resize_locked.das | Locked array operations — resize-while-iterating protection | |
| return_reference.das | Return reference — global ref, assign via ref, block returning ref | |
| rpipe.das | Right pipe `\|>` and `<\|` operator chaining | |
| failed_run_annotation_side_effects.das | `[run]` annotation side effects check | **expect** `40101` |
| safe_index.das | Safe index `?[]` on arrays, tables, vectors, strings | |
| safe_ptr_at.das | Safe index `?[]` on pointer types — null safety, struct pointer arrays | |
| safe_operators.das | Custom `operator []`, `?[]`, `.`, `?.` on user struct | |
| scatter_gather.das | SIMD gather/scatter ops — int, uint, float arrays with uint4 indices | |
| serialization.das | Archive serialization — structs, custom serialize, arrays, tables | |
| set_table.das | `table<int>` as set — insert, erase, keys iteration, clone, literal | |
| setand_and_setor_bool.das | Short-circuit `\|\|=` and `&&=` operators | |
| shifts.das | Bit shift operators — <<, >>, <<<, >>> for int/uint/int64/uint64 | |
| simple_string.das | String pass semantics — by value, by ref, clone, return, struct fields | |
| failed_sizeof_reference.das | `sizeof` on reference types | **expect** `39902:2` |
| smart_ptr.das | `smart_ptr<TestObjectSmart>` — scope, move, ref count, clone, use_count | |
| failed_smart_ptr_move.das | Unsafe `<-` move on `smart_ptr<>` type | **expect** `31300:1` |
| sort.das | Sort on arrays/fixed_arrays — int, uint, int64, float, double, string, custom struct, vector | |
| static.das | Static class members and methods | |
| failed_static_assert_in_infer.das | Static assertion during type inference | **expect** `40100` |
| static_if.das | `static_if` with `has_field`, const false elimination | |
| stdvec_r2v.das | `testFooArray()` — read-to-value iteration over std::vector binding | |
| storage_types.das | int8, uint8, int16, uint16 storage struct — sizeof and roundtrip | |
| strict_smart_ptr.das | strict_smart_pointers — emplace, inscope, smart_ptr operations | |
| string_builder.das | String interpolation `{expr}` — nested, escaped braces | |
| string_ops.das | String operations — case, find, strip, slice, conversions, fmt, iterator | |
| struct.das | Struct operations — pointers, null-safe chains, typeinfo, variants | |
| failed_structure_already_defined.das | Duplicate struct definition | **expect** `30206` |
| failed_structure_field_already_declared.das | Duplicate struct field name | **expect** `30115` |
| failed_structure_not_found_ambiguous.das | Ambiguous struct name — same name in two modules | **expect** `30302` |
| super.das | `super` keyword — parent constructor calls, parent method calls, 3-level hierarchy | |
| table.das | Table tombstone handling and iteration | |
| table_operations.das | Table find, insert, delete, key_exists, erase collision, lock panic, defaults, modify | |
| test_value_table_key.das | `table<EntityId; string>` — value-type table key ops, set operations | |
| testing_tools.das | Faker, fuzzer, testing_boost tools | |
| to_array.das | `to_array` — from fixed_array, range, each(), static/dynamic arrays | |
| to_table.das | `to_table` — from fixed_array of tuples | |
| trailing_delimiters.das | Trailing commas — arrays, structs, tables, block syntax | |
| try_recover.das | `try`/`recover` — null deref recovery, panic recovery | |
| tuple.das | Tuple — creation, named/positional fields, sizeof, clone, fixed_array | |
| tuple_expansion.das | Tuple expansion — `let (i, s, f) = foo()` | |
| failed_type_loop.das | Recursive type definition loop | **expect** `41000` |
| failed_type_not_found.das | Unknown type name error | **expect** `30301` |
| typeAlias.das | Type aliases — `Point3Array`, indexing, swizzle, array ops on aliased types | |
| typefunction.das | `[type_function]` annotation — `type<T>` argument syntax | |
| typeinfo.das | typeinfo sizeof, has_field, struct_get_annotation_argument | |
| typeinfo_annotations.das | Struct annotation queries — has_annotation, get_annotation_argument | |
| typeinfo_traits.das | typeinfo trait queries — is_local, is_ref, is_numeric, etc. | |
| typename.das | `typeinfo typename` for various types — generics, arrays, tables | |
| types.das | Global, local, argument, block variable types — int, float, string | |
| failed_unused_argument.das | Unused function argument warnings | **expect** `40206:2` |
| unused_arguments_annotation.das | `[unused_argument(b, c)]` annotation suppresses warnings | |
| utility_patterns.das | defer, static_let utility patterns | |
| variant.das | Variant type — typedef, struct sugar, `is`/`as`, `?as`, IorA, clone | |
| variants.das | Variant type — construction, match, access | |
| vec_constructors.das | Vector constructors — float/int/uint 2/3/4, range, type conversions | |
| vec_index.das | Vector indexing — [], ?[], out-of-range, array-of-vectors | |
| vec_ops.das | Vector arithmetic — *=, /= with scalar for float/int/uint | |
| vec_swizzle.das | Vector swizzle — .xy, .yx, .xyz, struct member vectors | |
| vector_fields.das | float4 .r/.g/.b/.a fields and swizzle | |
| with_statement.das | `with (struct) { field = val }` block scoping | |

## linq/

| File | Description | Expects errors |
|---|---|---|
| _common.das | *(helper)* Shared module — ComplexType, Person, Pet test data | |
| test_linq.das | to_table, core LINQ operations | |
| test_linq_aggregation.das | count, long_count, sum, average, min, max | |
| test_linq_bugs.das | LINQ bug fixes — unique_inplace empty, edge cases | |
| test_linq_concat.das | append, prepend, concat operations | |
| test_linq_element.das | element_at, first, last, single | |
| test_linq_fold.das | Comprehension fold, method chaining syntax | |
| test_linq_from_decs.das | LINQ querying decs entities | |
| test_linq_generation.das | default_empty, repeat, range generation | |
| test_linq_join.das | join, group_join operations | |
| test_linq_partition.das | skip, take, skip_while, take_while | |
| test_linq_querying.das | any, all, contains, where | |
| test_linq_set.das | distinct, union, intersect, except | |
| test_linq_sorting.das | reverse, order_by, order_by_descending | |
| test_linq_transform.das | select_transform, select_many, zip, cast | |

## lpipe/

| File | Description | Expects errors |
|---|---|---|
| test_pipes.das | lpipe macro — pipe into function calls, chain operators | |

## match/

| File | Description | Expects errors |
|---|---|---|
| all_matches.das | match/multi_match/static_match on enums, variants, structs, tuples | |
| test_match_edge.das | Match edge cases — nested, empty struct, wildcard, bindings, guards | |

## module_tests/

| File | Description | Expects errors |
|---|---|---|
| test_modules.das | Module system integration — compiles and runs 7 module scenarios via compile_file + make_file_access | |
| _modules/ | *(helper directory)* Module source files for test_modules.das (dastest skips `_`-prefixed dirs) | |

## math/

| File | Description | Expects errors |
|---|---|---|
| fake_numeric.das | *(helper)* Macro module — FakeNumericMacro for comprehensive faker tests | |
| inf_and_nan.das | Infinity and NaN comparisons | |
| math_matrix.das | Matrix operations — multiply, equality, transpose with fuzzing | |
| math_numeric.das | Numeric math functions (sin, cos, tan, etc.) via fake_numeric macro | |
| mat_ctors.das | Matrix constructors — float3x3, float3x4, float4x4, identity, sequence, row verification | |
| mat_let_handle.das | Matrix let/handle — float3x4 element access, transpose folding | |
| math_misc.das | Misc math — min, lerp, reflect, dot, length, cross, noise, ceili, saturate | |
| math_pack_unpack.das | pack_float_to_byte / unpack_byte_to_float with fuzzing | |
| math_quaternions.das | Quaternion operations with fuzzing | |

## regex/

| File | Description | Expects errors |
|---|---|---|
| test_regex_api.das | regex_match API — match, offset, empty string | |
| test_regex_basic.das | Basic regex — literal, dot, star, plus, question mark | |
| test_regex_charclass.das | Regex character classes — \w, \d, \s, negation, ranges | |
| test_regex_edge.das | Regex edge cases — offset parameter, greedy/lazy, empty pattern | |
| test_regex_newfeatures.das | Regex new features — ^, $, anchors, grouping | |
| test_regex_phase2.das | regex_search, regex_find_all, regex_replace | |
| test_regex_phase3.das | Dot-newline, regex_replace with backreferences | |
| test_strings_reexport.das | `require daslib/regex` re-exports strings module functions | |

## safe_addr/

| File | Description | Expects errors |
|---|---|---|
| test_shared_addr.das | shared_addr — address of shared globals, iteration | |

## soa/

| File | Description | Expects errors |
|---|---|---|
| test_soa_basic.das | `[soa]` struct — creation, push, length, field access | |
| test_soa_container.das | SOA container operations — erase, pop, resize, reserve, clear | |
| test_soa_iteration.das | SOA iteration — for loop, index access | |
| test_soa_noncopyable.das | SOA with non-copyable fields (array&lt;int&gt;) — push, erase, iterate | |

## spoof/

| File | Description | Expects errors |
|---|---|---|
| basic_spoof.das | spoof_template — value substitution (number, literal, expression, string) | |
| spoof_linked_list.das | TLinkedList spoof template — linked list iteration | |

## stbimage/

> **Note:** These tests require the stbimage module (`DAS_STBIMAGE_DISABLED=OFF`). Some tests also require stbtruetype (`DAS_STBTRUETYPE_DISABLED=OFF`).

| File | Description | Expects errors |
|---|---|---|
| _gen_test_images.das | *(helper)* Generate 64x64 test PNG/JPG/BMP images for file-based tests | |
| test_boost.das | stbimage_boost — safe image API, load/save wrappers, pixel access, format conversion | |
| test_convert.das | Image format conversion — RGB↔RGBA, greyscale, channel operations | |
| test_load_file.das | File-based image loading — PNG, JPG, BMP from disk | |
| test_load_info.das | Image info queries — dimensions, channels, format detection without full load | |
| test_raster.das | Rasterization — draw_line, draw_rect, fill_rect, draw_circle, pixel blending | |
| test_raster_blit.das | Blit operations — copy, scale, alpha-blend, region transfer | |
| test_resize.das | Image resize — nearest, bilinear, bicubic, different target sizes | |
| test_settings.das | Image load/save settings — quality, compression, flip | |
| test_stbimage_ttf.das | stbimage_ttf — safe TrueType font rendering via stbimage_boost + stbtruetype | |
| test_stbtruetype.das | stbtruetype — font loading, glyph metrics, bitmap rendering, SDF | |
| test_write.das | Image write — PNG, BMP, JPG, TGA output with verification | |

## strings/

| File | Description | Expects errors |
|---|---|---|
| delete_strings.das | delete_string — const heap string, heap string, empty string | |
| delete_strings_persistent.das | Same as delete_strings but with persistent_heap option | |
| strings_as_array.das | peek_data, modify_data — string as uint8 array | |
| strings_boost_split_and_join.das | split, join, and related boost functions with fuzzing | |
| strings_builder.das | String builder — write, write_char, write_escape_string with fuzzing (format fuzz disabled) | |
| strings_charset.das | Charset operations — is_char_in_set, set_total, set_element | |
| strings_convert.das | Numeric string conversions — int, float, out-of-range, errors | |
| strings_distance.das | levenshtein_distance with fuzzing | |
| strings_hash.das | Hash on das_string, hash_builder, builder vs direct hash | |
| strings_levenshtein.das | levenshtein_distance_fast with fuzzing | |
| strings_modifications.das | repeat, strip, replace, escape/unescape with fuzzing | |
| strings_properties.das | length, empty, starts_with, ends_with with fuzzing | |
| strings_replace_multiple.das | replace_multiple — multiple replacements, special chars, empty | |
| strings_search.das | find (string and char) with fuzzing | |
| strings_traits.das | is_alpha, is_new_line, is_white_space, is_number with fuzzing | |
| temporary_intern_strings.das | temp_string with intern_strings option | |
| temporary_strings.das | temp_string, build_temp_string — no heap allocations | |
| temporary_strings_failed.das | temp_string rejects variables and function calls | **expect** `40102:3` |
| test_bug_fixes.das | C++ bug fixes — chop bounds checking, edge cases | |
| test_cpp_functions.das | C++ string functions — fmt, safe_unescape, to_lower_in_place, character_at | |
| test_new_string_functions.das | New string utilities — contains, count_chars, pad_left/right, trim_chars | |
| test_strings_boost_extra.das | Extra strings_boost — wide, is_character_at, eq, join overloads | |

## template/

| File | Description | Expects errors |
|---|---|---|
| test_template.das | `[template]` annotation — `type<T>` and `decltype` as template arguments | |

## type_traits/

| File | Description | Expects errors |
|---|---|---|
| test_bitfields.das | bitfield_trait — each_bit_name iteration | |
| test_traits.das | type_traits — fields_count for struct/derived struct | |

## unsafe/

| File | Description | Expects errors |
|---|---|---|
| unsafe_reference.das | Local reference to non-local expression is unsafe | **expect** `31300` |

## uri/

| File | Description | Expects errors |
|---|---|---|
| test_uri.das | URI parsing, normalize, rebase, query params, edge cases | |

## verify/

| File | Description | Expects errors |
|---|---|---|
| test_verify_completion.das | `[verify_completion]` — rejects unbounded loops, recursion, keys() | **expect** `40104:8` |

---

## Summary

- **47** test directories (2 empty: `_dasHV/`, `jitted_scripts/`)
- **491** `.das` files total (~20 helper/module files, ~471 test files)
- **26** files with `expect` directives (expected compile errors), plus ~46 `failed_*` variants in `language/`
