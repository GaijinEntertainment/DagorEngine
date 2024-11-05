options rtti=true

require fio
require rtti
require ast
require math
require uriparser
require strings
require jobque
require daslib/ast_boost
require daslib/strings_boost
require daslib/rst_comment
require daslib/rst
require daslib/functional
require daslib/json
require daslib/json_boost
require daslib/regex
require daslib/regex_boost
require daslib/apply
require daslib/algorithm
require daslib/jobque_boost
require daslib/apply_in_context
require daslib/contracts
require daslib/defer
require daslib/if_not_null
require daslib/templates
require daslib/templates_boost
require daslib/instance_function
require daslib/is_local
require daslib/safe_addr
require daslib/static_let
require daslib/lpipe
require daslib/random
require daslib/math_bits
require daslib/math_boost
require daslib/array_boost
require daslib/archive
require daslib/unroll
require daslib/assert_once
require daslib/ast_block_to_loop
require daslib/ast_used
require daslib/constant_expression
require daslib/uriparser_boost
require daslib/sort_boost
require daslib/enum_trait
require daslib/cpp_bind
require daslib/decs
require daslib/decs_boost
require daslib/utf8_utils
require daslib/coroutines
require daslib/interfaces
require daslib/export_constructor
require daslib/faker
require daslib/fuzzer
require daslib/match

def document_module_math(root:string)
    var mod = get_module("math")
    var groups <- [{DocGroup
        group_by_regex("all numerics (uint*, int*, float*, double)", mod, %regex~(min|max)$%%);
        group_by_regex("float* and double", mod, %regex~(sign|abs|acos|asin|atan|atan2|cos|sin|tan|exp|log|exp2|
log2|pow|sqrt|rcp|ceil|floor|abs|saturate|sincos|is_finite|is_nan)$%%);
        group_by_regex("float* only", mod, %regex~(atan_est|atan2_est|rcp_est|ceili|floori|roundi|trunci|round|rsqrt|rsqrt_est|fract|\-)$%%);
        group_by_regex("float3 only", mod, %regex~(cross|reflect|refract|distance|distance_sq|
inv_distance|inv_distance_sq)$%%);
        group_by_regex("float2, float3, float4", mod, %regex~(length|length_sq|inv_length|inv_length_sq|
dot|normalize|fast_normalize)$%%);
        group_by_regex("Noise functions", mod, %regex~(uint32_hash|uint_noise_1D|uint_noise_2D|uint_noise_3D)$%%);
        group_by_regex("lerp/mad/clamp", mod, %regex~(lerp|mad|clamp)$%%);
        group_by_regex("Matrix operations", mod, %regex~(\=\=|\!\=|\*)$%%);
        group_by_regex("Matrix initializers", mod, %regex~(float3x4|float4x4|float3x3|identity3x4|identity4x4|identity3x3)$%%);
        group_by_regex("Matrix manipulation", mod, %regex~(identity|inverse|rotate|transpose|translation|compose|decompose|look_at|orthonormal_inverse|persp_forward|persp_reverse)$%%);
        group_by_regex("Quaternion operations", mod, %regex~(quat_conjugate|quat_mul|quat_mul_vec|quat_from_unit_arc|quat_from_unit_vec_ang)$%%);
        group_by_regex("Packing and unpacking", mod, %regex~(pack_float_to_byte|unpack_byte_to_float)$%%)
    }]
    document("Math library",mod,"{root}/math.rst","{root}/detail/math.rst",groups)

def document_module_builtin(root:string)
    var mod = get_module("$")
    var groups <- [{DocGroup
        hide_group(group_by_regex("Internal builtin functions", mod, %regex~builtin%%));
        hide_group(group_by_regex("Internal pointer arithmetics", mod, %regex~i_das_%%));
        hide_group(group_by_regex("Internal clone infrastructure", mod, %regex~clone%%));
        hide_group(group_by_regex("Internal finalize infrastructure", mod, %regex~finalize%%));
        group_by_regex("Containers", mod, %regex~(capacity|clear|length|resize|resize_no_init|reserve|each|emplace|erase|find|
find_for_edit|find_if_exists|find_index|find_index_if|has_value|key_exists|keys|values|lock|each_enum|each_ref|
find_for_edit_if_exists|lock_forever|next|nothing|pop|push|push_clone|back|sort|to_array|to_table|to_array_move|
to_table_move|empty|subarray|insert|move_to_ref|copy_to_local|move_to_local|get|remove_value)$%%);
        group_by_regex("Character set groups", mod, %regex~(is_alpha|is_number|is_white_space|is_char_in_set)$%%);
        group_by_regex("das::string manipulation", mod, %regex~(peek|set)$%%);
        group_by_regex("String builder", mod, %regex~(build_string|write|write_char|write_chars|write_escape_string)$%%);
        group_by_regex("Heap reporting", mod, %regex~(heap|string_heap|memory_report)%%);
        group_by_regex("Vector and matrix math", mod, %regex~(float3x4|float4x4|identity|inverse|rotate|translation|transpose)$%%);
        group_by_regex("GC0 infrastructure", mod, %regex~gc0%%);
        group_by_regex("Smart ptr infrastructure", mod, %regex~(add_ptr_ref|smart_ptr|get_const_ptr|move|move_new|get_ptr$)%%);
        group_by_regex("Macro infrastructure", mod, %regex~(is_folding|is_compiling|is_in_completion|is_reporting_compilation_errors)%%);
        group_by_regex("Profiler", mod, %regex~(profile|reset_profiler|dump_profile_info|collect_profile_info)$%%);
        group_by_regex("System infastructure", mod, %regex~(panic|print|sprint|sprint_json|to_log|to_compiler_log|error|terminate|breakpoint|stackwalk|get_das_root|is_in_aot|aot_enabled|is_intern_strings|eval_main_loop)$%%);
        group_by_regex("Memory manipulation", mod, %regex~(intptr|memcmp|variant_index|set_variant_index|hash|memcpy|lock_data|map_to_array|map_to_ro_array)$%%);
        group_by_regex("Binary serializer", mod, %regex~(binary_load|binary_save)$%%);
        group_by_regex("Path and command line", mod, %regex~(get_command_line_arguments)$%%);
        group_by_regex("Time and date", mod, %regex~(get_time_usec|ref_time_ticks|get_clock|get_time_nsec|mktime)$%%);
        group_by_regex("Lock checking", mod, %regex~(lock_count|set_verify_array_locks|set_verify_table_locks)$%%);
        group_by_regex("Lock checking internals", mod, %regex~(_move_with_lockcheck|_return_with_lockcheck|_at_with_lockcheck)$%%);
        group_by_regex("Bit operations", mod, %regex~(popcnt|clz|ctz|mul128)$%%);
        group_by_regex("Intervals", mod, %regex~(interval)$%%);
        group_by_regex("RTTI", mod, %regex~(class_rtti_size)$%%);
        hide_group(group_by_regex("Jit",mod,%regex~(invoke_code|.*jit.*)$%%));
        group_by_regex("Lock verification", mod, %regex~(set_verify_context_locks)$%%);
        group_by_regex("Initialization and finalization", mod, %regex~(using|clone|finalize)$%%);
        group_by_regex("Algorithms", mod, %regex~(swap|iter_range|count|ucount)$%%);
        group_by_regex("Memset", mod, %regex~(memset.*)$%%);
        group_by_regex("Malloc", mod, %regex~(malloc|free|malloc_usable_size)$%%);
    }]
    document("Built-in runtime",mod,"{root}/builtin.rst","{root}/detail/builtin.rst",groups)

def document_module_fio(root:string)
    var mod = get_module("fio")
    var groups <- [{DocGroup
        hide_group(group_by_regex("Internal builtin functions", mod, %regex~builtin%%));
        group_by_regex("File manipulation", mod, %regex~(f|stat$|getchar|remove|rename)%%);
        group_by_regex("Path manipulation", mod, %regex~(base_name|dir_name|get_full_file_name)$%%);
        group_by_regex("Directory manipulation", mod, %regex~(dir|mkdir|chdir|getcwd)$%%);
        group_by_regex("OS specific routines", mod, %regex~(sleep|exit|popen|popen_binary|get_env_variable|sanitize_command_line)$%%)
    }]
    document("File input output library",mod,"{root}/fio.rst","{root}/detail/fio.rst",groups)

def document_module_random(root:string)
    var mod = get_module("random")
    var groups <- [{DocGroup
        group_by_regex("Seed and basic generators", mod, %regex~(random_seed|random_seed2D|random_float|random_float4|random_int|random_big_int|random_int4|random_uint)$%%);
        group_by_regex("Random iterators", mod, %regex~(each_random_.*)$%%);
        group_by_regex("Specific distributions", mod, %regex~(random_unit_vector|random_in_unit_disk|random_in_unit_sphere)$%%)
    }]
    document("Random generator library",mod,"{root}/random.rst","{root}/detail/random.rst",groups)

def document_module_network(root:string)
    var mod = get_module("network")
    var groups <- [{DocGroup
        group_by_regex("Low lever NetworkServer IO", mod, %regex~(make_server|server_init|server_is_open|server_is_connected|server_tick|server_send|server_restore)$%%)
    }]
    document("Network socket library",mod,"{root}/network.rst","{root}/detail/network.rst",groups)

def document_module_uriparser(root:string)
    var mod = get_module("uriparser")
    var groups <- [{DocGroup
        group_by_regex("Initialization and finalization", mod, %regex~(Uri|using|clone|finalize)$%%);
        group_by_regex("Escape and unescape", mod, %regex~(escape_uri|unescape_uri)$%%);
        group_by_regex("Uri manipulations", mod, %regex~(normalize_uri|unescape_uri|string|uri_for_each_query_kv|remove_base_uri|add_base_uri|strip_uri|normalize)$%%);
        group_by_regex("File name conversions", mod, %regex~(to_unix_file_name|to_windows_file_name|to_file_name|uri_from_file_name|uri_from_windows_file_name|uri_from_unix_file_name|uri_to_unix_file_name|uri_to_windows_file_name|unix_file_name_to_uri|windows_file_name_to_uri|uri_to_file_name|file_name_to_uri)$%%);
        group_by_regex("GUID", mod, %regex~(make_new_guid)$%%)
    }]
    document("URI manipulation library based on UriParser",mod,"{root}/uriparser.rst","{root}/detail/uriparser.rst",groups)

def document_module_rtti(root:string)
    var mod = get_module("rtti")
    var groups <- [{DocGroup
        group_by_regex("Initialization and finalization", mod, %regex~(using|LineInfo|CodeOfPolicies|RttiValue_nothing)$%%);
        group_by_regex("Type access", mod, %regex~(get_dim|builtin_is_same_type|is_compatible_cast|get_das_type_name|is_same_type|each_dim|arg_types|arg_names|get_type_size|get_type_align)$%%);
        group_by_regex("Rtti context access", mod, %regex~(get_total_functions|get_total_variables|get_function_info|get_variable_info|get_variable_value|get_function_by_mnh|get_line_info|this_context|context_for_each_function|context_for_each_variable|class_info|type_info)$%%);
        group_by_regex("Program access", mod, %regex~(program_for_each_module|program_for_each_registered_module|get_this_module|get_module)$%%);
        group_by_regex("Module access", mod, %regex~(module_for_each_structure|module_for_each_enumeration|module_for_each_function|module_for_each_generic|module_for_each_global|module_for_each_annotation)$%%);
        group_by_regex("Annotation access", mod, %regex~(get_annotation_argument_value|add_annotation_argument)$%%);
        group_by_regex("Compilation and simulation", mod, %regex~(compile|compile_file|for_each_expected_error|for_each_require_declaration|simulate)$%%);
        group_by_regex("File access", mod, %regex~(make_file_access|set_file_source|add_file_access_root)$%%);
        group_by_regex("Structure access", mod, %regex~(rtti_builtin_structure_for_each_annotation|basic_struct_for_each_field|structure_for_each_annotation|basic_struct_for_each_parent)$%%);
        group_by_regex("Data walking and printing", mod, %regex~(sprint_data|describe|get_mangled_name)$%%);
        group_by_regex("Function and mangled name hash", mod, %regex~(get_function_by_mangled_name_hash|get_function_mangled_name_hash|get_function_address)$%%);
        group_by_regex("Context and mutex locking", mod, %regex~(lock_this_context|lock_context|lock_mutex)$%%);
        group_by_regex("Runtime data access", mod, %regex~(get_table_key_index)$%%);
    }]
    document("Runtime type information library",mod,"{root}/rtti.rst","{root}/detail/rtti.rst",groups)

def document_module_ast(root:string)
    var mod = get_module("ast")
    var groups <- [{DocGroup
        group_by_regex("Call generation", mod, %regex~(make_call)$%%);
        group_by_regex("Visitor pattern", mod, %regex~visit.*%%);
        group_by_regex("Expression generation", mod, %regex~(force_generated|get_expression_annotation|make_type_info_structure)%%);
        group_by_regex("Adapter generation", mod, %regex~make_.*%%);
        group_by_regex("Adapter application", mod, %regex~(add_.*_(annotation|macro)|add_new_comment_reader|add_comment_reader|add_module_option)%%);
        group_by_regex("Adding objects to objects", mod, %regex~add_.*%%);
        group_by_regex("Program and module access", mod, %regex~(this_program|this_module|compiling_program|compiling_module)$%%);
        group_by_regex("Textual descriptions of the objects", mod, %regex~(describe.*|das_to_string)%%);
        group_by_regex("Searching", mod, %regex~find.*%%);
        group_by_regex("Iterating", mod, %regex~(for_each.*|.*_foreach)%%);
        group_by_regex("Cloning", mod, %regex~clone.*%%);
        group_by_regex("Mangled name", mod, %regex~(parse_mangled_name|get_mangled_name)%%);
        group_by_regex("Size and offset", mod, %regex~.*_(size|offset)%%);
        group_by_regex("Pointer conversion", mod, %regex~(ExpressionPtr|FunctionPtr|StructurePtr)%%);
        group_by_regex("Evaluations", mod, %regex~(eval_single_expression)%%);
        group_by_regex("Error reporting", mod, %regex~(macro_error)%%);
        group_by_regex("Location and context", mod, %regex~(force_at|collect_dependencies|get_ast_context)%%);
        // hide_group(group_by_regex("Jit support",mod,%regex~(make_interop_node|get_builtin_function_address)%%));
        group_by_regex("Use queries", mod, %regex~(get_use_global_variables|get_use_functions)%%);
        group_by_regex("Log", mod, %regex~(to_compilation_log)%%);
        group_by_regex("Removal", mod, %regex~(remove.*)%%);
        group_by_regex("Properties", mod, %regex~(is|has|get|can)_.*%%);
        group_by_regex("Infer", mod, %regex~(infer_generic_type|update_alias_map)$%%);
        hide_group(group_by_regex("Internal ast infrastructure", mod, %regex~(builtin_ast_.*)%%))
    }]
    document("AST manipulation library",mod,"{root}/ast.rst","{root}/detail/ast.rst",groups)

def document_module_ast_boost(root:string)
    var mod = find_module("ast_boost")
    var groups <- [{DocGroup
        group_by_regex("Containers", mod, %regex~(emplace_new)%%);
        group_by_regex("Textual descriptions of the objects", mod, %regex~(describe.*|debug_expression)%%);
        group_by_regex("Queries", mod, %regex~(is.*|find.*|get.*)%%);
        group_by_regex("Annotations", mod, %regex~(append.*|add.*)%%);
        group_by_regex("Expression generation", mod, %regex~(override_method|panic_expr_as|make_static_assert_false|convert_to_expression)%%);
        group_by_regex("Visitors", mod, %regex~(visit.*)%%);
        group_by_regex("Type generation", mod, %regex~(function_to_type)%%);
        hide_group(group_by_regex("Clonning", mod, %regex~(clone)%%));
        group_by_regex("Setup", mod, %regex~setup.*$%%)
    }]
    document("Boost package for the AST",mod,"{root}/ast_boost.rst","{root}/detail/ast_boost.rst",groups)

def document_module_strings(root:string)
    var mod = find_module("strings")
    var groups <- [{DocGroup
        hide_group(group_by_regex("Internal builtin functions", mod, %regex~builtin%%));
        group_by_regex("Character set", mod, %regex~(set_total|set_element|is_char_in_set)$%%);
        group_by_regex("Character groups", mod, %regex~is.+%%);
        group_by_regex("Character by index", mod, %regex~(character_at|character_uat)$%%);
        group_by_regex("String properties", mod, %regex~(empty|ends_with|length|starts_with)$%%);
        group_by_regex("String builder", mod, %regex~(build_string|format|write|write_char|write_chars|write_escape_string|build_hash)$%%);
        group_by_regex("das::string manipulation", mod, %regex~(append|resize)$%%);
        group_by_regex("String modifications", mod, %regex~(chop|escape|unescape|repeat|replace|reverse|slice|
strip|strip_left|strip_right|to_lower|to_lower_in_place|to_upper|to_upper_in_place|rtrim|safe_unescape|ltrim|trim)$%%);
        group_by_regex("Search substrings", mod, %regex~find.*%%);
        group_by_regex("String conversion routines", mod, %regex~(double|float|int|uint|int64|uint64|to_double|to_float|to_int|to_uint|to_int64|to_uint64|string|to_char)$%%);
        group_by_regex("String as array", mod, %regex~(modify_data|peek_data)$%%);
        group_by_regex("Low level memory allocation", mod, %regex~(delete_string|reserve_string_buffer)$%%)
    }]
    document("String manipulation library",mod,"{root}/strings.rst","{root}/detail/strings.rst",groups)

def document_module_strings_boost(root:string)
    var mod = find_module("strings_boost")
    var groups <- [{DocGroup
        group_by_regex("Split and join", mod, %regex~(split|split_by_chars|join)$%%);
        group_by_regex("Formatting", mod, %regex~(wide)$%%);
        group_by_regex("Queries and comparisons", mod, %regex~(is_character_at|eq)$%%);
        group_by_regex("Replace", mod, %regex~(replace_multiple)$%%);
        group_by_regex("Levenshtein distance", mod, %regex~(levenshtein_distance|levenshtein_distance_fast)$%%);
        group_by_regex("Character traits", mod, %regex~(is_hex|is_tab_or_space)$%%);
    }]
    document("Boost package for string manipulation library",mod,"{root}/strings_boost.rst","{root}/detail/strings_boost.rst",groups)


def document_module_functional(root:string)
    var mod = find_module("functional")
    var groups <- [{DocGroup
        group_by_regex("Map, reduce", mod, %regex~(filter|map|reduce|any|sum|all|cycle|islice|count|repeat_ref|repeat|not|echo|flatten)$%%);
        group_by_regex("Queries", mod, %regex~(is_equal|is_not_equal)$%%)
    }]
    document("Functional programming library",mod,"{root}/functional.rst","{root}/detail/functional.rst",groups)

def document_module_json(root:string)
    var mod = find_module("json")
    var groups <- [{DocGroup
        group_by_regex("Value conversion", mod, %regex~(JV|JVNull)$%%);
        group_by_regex("Read and write", mod, %regex~(read_json|write_json)$%%);
        group_by_regex("JSON properties", mod, %regex~(set_no_trailing_zeros|set_no_empty_arrays|set_allow_duplicate_keys)$%%);
        group_by_regex("Broken JSON", mod, %regex~(try_fixing_broken_json)$%%)
    }]
    document("JSON manipulation library",mod,"{root}/json.rst","{root}/detail/json.rst",groups)

def document_module_json_boost(root:string)
    var mod = find_module("json_boost")
    var groups <- [{DocGroup
        group_by_regex("Value conversion", mod, %regex~(JV|from_JV)$%%);
        group_by_regex("stub1", mod, %regex~(stub1)$%%)
    }]
    document("Boost package for JSON",mod,"{root}/json_boost.rst","{root}/detail/json_boost.rst",groups)

def document_module_regex(root:string)
    var mod = find_module("regex")
    var groups <- [{DocGroup
        group_by_regex("Compilation and validation", mod, %regex~(regex_compile|is_valid|regex_debug|debug_set|visit_top_down)$%%);
        group_by_regex("Access", mod, %regex~(regex_group|regex_foreach)$%%);
        group_by_regex("Match", mod, %regex~(regex_match)$%%);
        group_by_regex("Generation", mod, %regex~(re_gen.*)$%%)
    }]
    document("Regular expression library",mod,"{root}/regex.rst","{root}/detail/regex.rst",groups)

def document_module_regex_boost(root:string)
    var mod = find_module("regex_boost")
    var groups <- [{DocGroup
        group_by_regex("stub0", mod, %regex~(stub0)$%%);
        group_by_regex("stub1", mod, %regex~(stub1)$%%)
    }]
    document("Boost package for REGEX",mod,"{root}/regex_boost.rst","{root}/detail/regex_boost.rst",groups)

def document_module_rst(root:string)
    var mod = find_module("rst")
    var groups <- [{DocGroup
        group_by_regex("Document writers", mod, %regex~document%%);
        group_by_regex("Descriptions", mod, %regex~describe%%);
        group_by_regex("Label makers", mod, %regex~\w+_label_name%%);
        group_by_regex("RST section makers", mod, %regex~make%%);
        group_by_regex("Documentaton queries", mod, %regex~.+_needs%%);
        group_by_regex("Group operations", mod, %regex~.*group%%);
        group_by_regex("Naming helpers", mod, %regex~(topic|module_name|function_name)$%%)
    }]
    document("Documentation generator",mod,"{root}/rst.rst","{root}/detail/rst.rst",groups)

def document_module_apply(root:string)
    var mod = find_module("apply")
    var groups <- [{DocGroup
        group_by_regex("stub0", mod, %regex~(stub0)$%%);
        group_by_regex("stub1", mod, %regex~(stub1)$%%)
    }]
    document("Apply reflection pattern",mod,"{root}/apply.rst","{root}/detail/apply.rst",groups)

def document_module_jobque(root:string)
    var mod = find_module("jobque")
    var groups <- [{DocGroup
        group_by_regex("Channel, JobStatus, Lockbox", mod, %regex~(append|notify|join|channel_create|channel_remove|add_ref|release|notify_and_release|lock_box_create|lock_box_remove|job_status_create|job_status_remove)$%%);
        group_by_regex("Queries", mod, %regex~(get_total_hw_jobs|get_total_hw_threads|is_job_que_shutting_down)$%%);
        group_by_regex("Internal invocations", mod, %regex~(new_job_invoke|new_thread_invoke|new_debugger_thread)$%%);
        group_by_regex("Construction", mod, %regex~(with_channel|with_job_status|with_job_que|with_lock_box)$%%);
        group_by_regex("Atomic", mod, %regex~(atomic32_create|atomic32_remove|with_atomic32|atomic64_create|atomic64_remove|with_atomic64|get|set|inc|dec)$%%)
    }]
    document("Jobs and threads",mod,"{root}/jobque.rst","{root}/detail/jobque.rst",groups)

def document_module_jobque_boost(root:string)
    var mod = find_module("jobque_boost")
    var groups <- [{DocGroup
        group_by_regex("Invocations", mod, %regex~(new_job|new_thread)$%%);
        group_by_regex("Iteration", mod, %regex~(for_each|each)$%%);
        group_by_regex("Passing data", mod, %regex~(push|push_clone)$%%);
        group_by_regex("Internal capture details", mod, %regex~(capture_jobque_channel|capture_jobque_job_status|release_capture_jobque_channel|release_capture_jobque_job_status)$%%)
    }]
    document("Boost package for jobs and threads",mod,"{root}/jobque_boost.rst","{root}/detail/jobque_boost.rst",groups)

def document_module_apply_in_context(root:string)
    var mod = find_module("apply_in_context")
    var groups <- [{DocGroup
        group_by_regex("stub0", mod, %regex~(stub0)$%%);
        group_by_regex("stub1", mod, %regex~(stub1)$%%)
    }]
    document("Cross-context evaluation helpers",mod,"{root}/apply_in_context.rst","{root}/detail/apply_in_context.rst",groups)

def document_module_algorithm(root:string)
    var mod = find_module("algorithm")
    var groups <- [{DocGroup
        group_by_regex("Search", mod, %regex~(lower_bound|binary_search)$%%);
        group_by_regex("Array manipulation", mod, %regex~(unique|sort_unique|reverse|combine)$%%)
    }]
    document("Miscelanious algorithms",mod,"{root}/algorithm.rst","{root}/detail/algorithm.rst",groups)

def document_module_contracts(root:string)
    var mod = find_module("contracts")
    var groups <- [{DocGroup
        group_by_regex("Type queries", mod, %regex~(isYetAnotherVectorTemplate)$%%)
    }]
    document("Miscelanious contract annotations",mod,"{root}/contracts.rst","{root}/detail/contracts.rst",groups)

def document_module_defer(root:string)
    var mod = find_module("defer")
    var groups <- [{DocGroup
        group_by_regex("Defer", mod, %regex~(defer)$%%);
        group_by_regex("Stub", mod, %regex~(nada)$%%)
    }]
    document("defer and defer_delete macros",mod,"{root}/defer.rst","{root}/detail/defer.rst",groups)

def document_module_if_not_null(root:string)
    var mod = find_module("if_not_null")
    var groups <- [{DocGroup
        group_by_regex("stub0", mod, %regex~(stub0)$%%);
        group_by_regex("stub1", mod, %regex~(stub1)$%%)
    }]
    document("if_not_null macro",mod,"{root}/if_not_null.rst","{root}/detail/if_not_null.rst",groups)

def document_module_instance_function(root:string)
    var mod = find_module("instance_function")
    var groups <- [{DocGroup
        group_by_regex("stub0", mod, %regex~(stub0)$%%);
        group_by_regex("stub1", mod, %regex~(stub1)$%%)
    }]
    document("instance_function function annotation",mod,"{root}/instance_function.rst","{root}/detail/instance_function.rst",groups)

def document_module_templates(root:string)
    var mod = find_module("templates")
    var groups <- [{DocGroup
        group_by_regex("stub0", mod, %regex~(stub0)$%%);
        group_by_regex("stub1", mod, %regex~(stub1)$%%)
    }]
    document("decltype macro and template function annotation",mod,"{root}/templates.rst","{root}/detail/templates.rst",groups)

def document_module_templates_boost(root:string)
    var mod = find_module("templates_boost")
    var groups <- [{DocGroup
        group_by_regex("Template rules", mod, %regex~(kaboomVarField|replaceVariable|renameVariable|replaceType|replaceAnnotationArgument|replaceBlockArgument)$%%);
        group_by_regex("Template application", mod, %regex~(apply_template)$%%);
        group_by_regex("Expression helpers", mod, %regex~(remove_deref)$%%);
        group_by_regex("Block helpers", mod, %regex~(unquote_block|move_unquote_block)$%%);
        group_by_regex("Global variable helpers", mod, %regex~(add_global_var|add_global_let|add_global_private_var|add_global_private_let)$%%);
        group_by_regex("Hygenic names", mod, %regex~(make_unique_private_name)$%%)
    }]
    document("Template application helpers",mod,"{root}/templates_boost.rst","{root}/detail/templates_boost.rst",groups)

def document_module_is_local(root:string)
    var mod = find_module("is_local")
    var groups <- [{DocGroup
        group_by_regex("Scope checks", mod, %regex~(is_local_expr|is_local_or_global_expr|is_scope_expr)$%%)
    }]
    document("is_local_xxx ast helpers",mod,"{root}/is_local.rst","{root}/detail/is_local.rst",groups)

def document_module_safe_addr(root:string)
    var mod = find_module("safe_addr")
    var groups <- [{DocGroup
        group_by_regex("Safe temporary address", mod, %regex~(safe_addr|shared_addr)$%%);
        group_by_regex("Temporary pointers", mod, %regex~(temp_ptr)$%%)
    }]
    document("safe_addr macro",mod,"{root}/safe_addr.rst","{root}/detail/safe_addr.rst",groups)

def document_module_static_let(root:string)
    var mod = find_module("static_let")
    var groups <- [{DocGroup
        group_by_regex("Static variable declarations", mod, %regex~(static_let|static_let_finalize)$%%);
        group_by_regex("stub1", mod, %regex~(stub1)$%%)
    }]
    document("static_let macro",mod,"{root}/static_let.rst","{root}/detail/static_let.rst",groups)

def document_module_lpipe(root:string)
    var mod = find_module("lpipe")
    var groups <- [{DocGroup
        group_by_regex("stub0", mod, %regex~(stub0)$%%);
        group_by_regex("stub1", mod, %regex~(stub1)$%%)
    }]
    document("lpipe macro",mod,"{root}/lpipe.rst","{root}/detail/lpipe.rst",groups)

def document_module_math_bits(root:string)
    var mod = find_module("math_bits")
    var groups <- [{DocGroup
        group_by_regex("float in int,uint", mod, %regex~(int_bits_to_float|uint_bits_to_float)$%%);
        group_by_regex("int,uint in float", mod, %regex~(float_bits_to_int|float_bits_to_uint)$%%);
        group_by_regex("int64,uint64 in double", mod, %regex~(int64_bits_to_double|uint64_bits_to_double|double_bits_to_int64|double_bits_to_uint64)$%%);
        group_by_regex("bit-cast vec4f", mod, %regex~(cast_to.*)$%%);
    }]
    document("Math bit helpers",mod,"{root}/math_bits.rst","{root}/detail/math_bits.rst",groups)

def document_module_math_boost(root:string)
    var mod = find_module("math_boost")
    var groups <- [{DocGroup
        group_by_regex("Angle conversions", mod, %regex~(degrees|radians)$%%);
        group_by_regex("Intersections", mod, %regex~(is_intersecting)$%%);
        group_by_regex("Matrices", mod, %regex~(look_at_lh|look_at_rh|perspective_lh|perspective_rh|perspective_rh_opengl|ortho_rh|planar_shadow)$%%);
        group_by_regex("Plane", mod, %regex~(plane_dot|plane_normalize|plane_from_point_normal)$%%);
        group_by_regex("Color packig and unpacking", mod, %regex~(RGBA_TO_UCOLOR|UCOLOR_TO_RGBA|UCOLOR_TO_RGB)$%%)
    }]
    document("Boost package for math",mod,"{root}/math_boost.rst","{root}/detail/math_boost.rst",groups)

def document_module_array_boost(root:string)
    var mod = find_module("array_boost")
    var groups <- [{DocGroup
        group_by_regex("Temporary arrays", mod, %regex~(temp_array)$%%);
        group_by_regex("Empty check", mod, %regex~(empty)$%%)
    }]
    document("Boost package for array manipulation",mod,"{root}/array_boost.rst","{root}/detail/array_boost.rst",groups)

def document_module_archive(root:string)
    var mod = find_module("archive")
    var groups <- [{DocGroup
        group_by_regex("Serialization", mod, %regex~(serialize|serialize_raw|read_raw|write_raw)$%%);
        group_by_regex("Memory archive", mod, %regex~(mem_archive_save|mem_archive_load)$%%)
    }]
    document("General prupose serialization",mod,"{root}/archive.rst","{root}/detail/archive.rst",groups)

def document_module_unroll(root:string)
    var mod = find_module("unroll")
    var groups <- [{DocGroup
        group_by_regex("Unrolling", mod, %regex~(unroll)$%%)
    }]
    document("Loop unrolling",mod,"{root}/unroll.rst","{root}/detail/unroll.rst",groups)

def document_module_assert_once(root:string)
    var mod = find_module("assert_once")
    var groups <- [{DocGroup
        group_by_regex("Assertion", mod, %regex~(assert_once)$%%)
    }]
    document("Assert once",mod,"{root}/assert_once.rst","{root}/detail/assert_once.rst",groups)

def document_module_ast_block_to_loop(root:string)
    var mod = find_module("ast_block_to_loop")
    var groups <- [{DocGroup
        group_by_regex("Block to loop conversion", mod, %regex~(convert_block_to_loop)$%%)
    }]
    document("DECS, AST block to loop",mod,"{root}/ast_block_to_loop.rst","{root}/detail/ast_block_to_loop.rst",groups)

def document_module_ast_used(root:string)
    var mod = find_module("ast_used")
    var groups <- [{DocGroup
        group_by_regex("Collecting type information", mod, %regex~(collect_used_types)$%%)
    }]
    document("AST type ussage collection",mod,"{root}/ast_used.rst","{root}/detail/ast_used.rst",groups)

def document_module_constexpr(root:string)
    var mod = find_module("constant_expression")
    var groups <- [{DocGroup
        group_by_regex("Macro helpers", mod, %regex~(isConstantExpression)$%%)
    }]
    document("Constant expression checker and substitution",mod,"{root}/constexpr.rst","{root}/detail/constexpr.rst",groups)

def document_module_uriparser_boost(root:string)
    var mod = find_module("uriparser_boost")
    var groups <- [{DocGroup
        group_by_regex("Split and compose", mod, %regex~(uri_split_full_path|uri_compose_query|uri_compose_query_in_order|uri_compose)$%%);
        group_by_regex("Component accessors", mod, %regex~(scheme|user_info|host|port|path|query|fragment)$%%)
    }]
    document("Boost package for the URI parser",mod,"{root}/uriparser_boost.rst","{root}/detail/uriparser_boost.rst",groups)

def document_module_sort_boost(root:string)
    var mod = find_module("sort_boost")
    var groups <- [{DocGroup
        group_by_regex("stub", mod, %regex~(stub)$%%)
    }]
    document("Boost package for the builtin sort",mod,"{root}/sort_boost.rst","{root}/detail/sort_boost.rst",groups)

def document_module_enum_trait(root:string)
    var mod = find_module("enum_trait")
    var groups <- [{DocGroup
        group_by_regex("stub", mod, %regex~(stub)$%%)
    }]
    document("Enumeration traits",mod,"{root}/enum_trait.rst","{root}/detail/enum_trait.rst",groups)

def document_module_cpp_bind(root:string)
    var mod = find_module("cpp_bind")
    var groups <- [{DocGroup
        group_by_regex("Generation of bindings", mod, %regex~(log_cpp_class_adapter)$%%)
    }]
    document("C++ bindings generator",mod,"{root}/cpp_bind.rst","{root}/detail/cpp_bind.rst",groups)

def document_module_decs(root:string)
    var mod = find_module("decs")
    var groups <- [{DocGroup
        group_by_regex("Comparison and access", mod, %regex~(\=\=|\!\=|\.)$%%);
        group_by_regex("Access (get/set/clone)", mod, %regex~(has|get|set|clone|remove)$%%);
        group_by_regex("Deubg and serialization", mod, %regex~(describe|serialize|finalize|debug_dump)$%%);
        group_by_regex("Stages", mod, %regex~(register_decs_stage_call|decs_stage|commit)$%%);
        group_by_regex("Deferred actions", mod, %regex~(update_entity|create_entity|delete_entity)$%%);
        group_by_regex("GC and reset", mod, %regex~(before_gc|after_gc|restart)$%%);
        group_by_regex("Iteration", mod, %regex~(for_each_archetype|for_eid_archetype|for_each_archetype_find|get_ro|decs_array|get_default_ro|get_optional)$%%);
        group_by_regex("Request", mod, %regex~(verify_request|compile_request|lookup_request|EcsRequestPos)$%%)
    }]
    document("DECS, Daslang entity component system",mod,"{root}/decs.rst","{root}/detail/decs.rst",groups)

def document_module_decs_boost(root:string)
    var mod = find_module("decs_boost")
    var groups <- [{DocGroup
        group_by_regex("Stub1", mod, %regex~(stub1)$%%);
        group_by_regex("Stub2", mod, %regex~(stub2)$%%)
    }]
    document("Boost package for DECS",mod,"{root}/decs_boost.rst","{root}/detail/decs_boost.rst",groups)

def document_module_utf8_utils(root:string)
    var mod = find_module("utf8_utils")
    var groups <- [{DocGroup
        group_by_regex("stub", mod, %regex~(stub)$%%)
    }]
    document("UTF-8 utilities",mod,"{root}/utf8_utils.rst","{root}/detail/utf8_utils.rst",groups)

def document_module_macro_boost(root:string)
    var mod = find_module("macro_boost")
    var groups <- [{DocGroup
        group_by_regex("Implementation details", mod, %regex~(macro_verify)$%%)
    }]
    document("Boost package for the miscelanious macro manipulations",mod,"{root}/macro_boost.rst","{root}/detail/macro_boost.rst",groups)

def document_module_coroutines(root:string)
    var mod = find_module("coroutines")
    var groups <- [{DocGroup
        group_by_regex("Top level coroutine evaluation", mod, %regex~(cr_run|cr_run_all)$%%)
    }]
    document("Coroutines and additional generator support",mod,"{root}/coroutines.rst","{root}/detail/coroutines.rst",groups)

def document_module_interfaces(root:string)
    var mod = find_module("interfaces")
    var groups <- [{DocGroup
        group_by_regex("Top level coroutine evaluation", mod, %regex~(cr_run|cr_run_all)$%%)
    }]
    document("Interfaces",mod,"{root}/interfaces.rst","{root}/detail/interfaces.rst",groups)

def document_module_export_constructor(root:string)
    var mod = find_module("export_constructor")
    var groups <- [{DocGroup
        group_by_regex("Top level coroutine evaluation", mod, %regex~(cr_run|cr_run_all)$%%)
    }]
    document("Export constructor",mod,"{root}/export_constructor.rst","{root}/detail/export_constructor.rst",groups)

def document_module_faker(root:string)
    var mod = find_module("faker")
    var groups <- [{DocGroup
        group_by_regex("Constructor", mod, %regex~(Faker)$%%);
        group_by_regex("Random values", mod, %regex~random_.*$%%);
        group_by_regex("Random strings", mod, %regex~(any_.*|long_string|number|positive_int)$%%);
        group_by_regex("Date and time", mod, %regex~(month|day|date|is_leap_year|week_day)%%)
    }]
    document("Faker",mod,"{root}/faker.rst","{root}/detail/faker.rst",groups)

def document_module_fuzzer(root:string)
    var mod = find_module("fuzzer")
    var groups <- [{DocGroup
        group_by_regex("Fuzzer tests", mod, %regex~fuzz.*$%%)
    }]
    document("Fuzzer",mod,"{root}/fuzzer.rst","{root}/detail/fuzzer.rst",groups)

def document_module_match(root:string)
    var mod = find_module("match")
    var groups <- [{DocGroup
        hide_group(group_by_regex("Implementation details", mod, %regex~(match_type|match_expr)$%%))
    }]
    document("Pattern matching",mod,"{root}/match.rst","{root}/detail/match.rst",groups)


[export]
def main
    let root = get_das_root()+"/doc/source/stdlib"  // todo: modify to output /temp so that we can merge changes
    topic_root = "{root}/detail"
    // document embedded modules
    document_module_builtin(root)
    document_module_math(root)
    document_module_fio(root)
    document_module_network(root)
    document_module_uriparser(root)
    document_module_rtti(root)
    document_module_ast(root)
    document_module_strings(root)
    document_module_jobque(root)

    document_module_algorithm(root)
    document_module_apply_in_context(root)
    document_module_apply(root)
    document_module_archive(root)
    document_module_array_boost(root)
    document_module_assert_once(root)
    document_module_ast_block_to_loop(root)
    document_module_ast_boost(root)
    document_module_ast_used(root)
    document_module_constexpr(root)
    document_module_contracts(root)
    document_module_coroutines(root)
    document_module_cpp_bind(root)
    // document_module_cpp_gen(root)                // NOTE - do not document. its obsolete. use cbind_boost instead
    // document_module_dap(root)                    // todo: ask to document
    // document_module_das_source_formatter(root)   // todo: ask to document
    // document_module_debug(root)                  // todo: document???
    document_module_decs_boost(root)                 // DECS
    // document_module_decs_state(root)                 // DECS
    document_module_decs(root)                       // DECS
    document_module_defer(root)
    document_module_enum_trait(root)
    document_module_export_constructor(root)
    document_module_faker(root)
    document_module_functional(root)
    document_module_fuzzer(root)
    document_module_if_not_null(root)
    document_module_instance_function(root)
    document_module_interfaces(root)
    document_module_is_local(root)
    document_module_jobque_boost(root)
    document_module_json_boost(root)
    document_module_json(root)
    // document_module_lint_everything(root)        // todo: document?
    // document_module_lint(root)                   // todo: document?
    // document_module_live(root)
    document_module_lpipe(root)
    document_module_macro_boost(root)
    document_module_math_bits(root)
    document_module_math_boost(root)
    document_module_random(root)
    document_module_regex_boost(root)
    document_module_regex(root)
    document_module_rst(root)
    document_module_safe_addr(root)
    document_module_sort_boost(root)
    document_module_static_let(root)
    document_module_strings_boost(root)
    document_module_templates_boost(root)
    document_module_templates(root)
    document_module_unroll(root)
    document_module_uriparser_boost(root)
    document_module_utf8_utils(root)
    document_module_match(root)
