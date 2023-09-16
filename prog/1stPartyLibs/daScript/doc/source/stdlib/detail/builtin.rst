.. |typedef-builtin-print_flags| replace:: this bitfield specifies how exactly values are to be printed

.. |function-builtin-breakpoint| replace:: breakpoint will call os_debugbreakpoint, which is link-time unresolved dependency. It's supposed to call breakpoint in debugger tool, as sample implementation does.

.. |function-builtin-builtin_get_command_line_arguments| replace:: to be documented

.. |function-builtin-capacity| replace:: capacity will return current capacity of table or array `arg`. Capacity is the count of elements, allocating (or pushing) until that size won't cause reallocating dynamic heap.

.. |function-builtin-clear| replace:: clear will clear whole table or array `arg`. The size of `arg` after clear is 0.

.. |function-builtin-clone| replace:: to be documented

.. |function-builtin-clone_string| replace:: clones string `arg` and returns pointer to new string. this ensures string is actually allocated in the current context heap.

.. |function-builtin-collect_profile_info| replace:: enabling collecting of the use counts by built-in profiler

.. |function-builtin-dump_profile_info| replace:: dumps use counts of all lines collected by built-in profiler

.. |function-builtin-empty| replace:: returns true if iterator is empty, i.e. would not produce any more values or uninitialized

.. |function-builtin-gc0_reset| replace:: resets gc0 storage. stored pointers will no longer be accessible

.. |function-builtin-gc0_restore_ptr| replace:: restores pointer from gc0 storage by `name`

.. |function-builtin-gc0_restore_smart_ptr| replace:: restores smart_ptr from gc0 storage `name`

.. |function-builtin-gc0_save_ptr| replace:: saves pointer to gc0 storage by specifying `name`

.. |function-builtin-gc0_save_smart_ptr| replace:: saves smart_ptr to gc0 storage by specifying `name`

.. |function-builtin-get_clock| replace:: return a current calendar time. The value returned generally represents the number of seconds since 00:00 hours, Jan 1, 1970 UTC (i.e., the current unix timestamp).

.. |function-builtin-get_command_line_arguments| replace:: returns array of command line arguments.

.. |function-builtin-get_das_root| replace:: returns path to where `daslib` and other libraries exist. this is typically root folder of the daScript main repository

.. |function-builtin-hash| replace:: returns hash value of the `data`. current implementation uses FNV64a hash.

.. |function-builtin-heap_bytes_allocated| replace:: will return bytes allocated on heap (i.e. really used, not reserved)

.. |function-builtin-heap_depth| replace:: returns number of generations in the regular heap

.. |function-builtin-heap_report| replace:: reports heap usage and allocations

.. |function-builtin-heap_collect| replace:: calls garbage collection on the regular heap

.. |function-builtin-i_das_ptr_add| replace:: to be documented

.. |function-builtin-i_das_ptr_dec| replace:: to be documented

.. |function-builtin-i_das_ptr_diff| replace:: to be documented

.. |function-builtin-i_das_ptr_inc| replace:: to be documented

.. |function-builtin-i_das_ptr_set_add| replace:: to be documented

.. |function-builtin-i_das_ptr_set_sub| replace:: to be documented

.. |function-builtin-i_das_ptr_sub| replace:: to be documented

.. |function-builtin-is_compiling| replace:: returns true if context is being compiled

.. |function-builtin-is_folding| replace:: returns true if context is beeing folded, i.e during constant folding pass

.. |function-builtin-is_compiling_macros| replace:: returns true if context is being compiled and the compiler is currently executing macro pass

.. |function-builtin-is_compiling_macros_in_module| replace:: returns true if context is being compiled, its macro pass, and its in the specific module

.. |function-builtin-is_reporting_compilation_errors| replace:: returns true if context failed to compile, and infer pass is reporting compilation errors

.. |function-builtin-length| replace:: length will return current size of table or array `arg`.

.. |function-builtin-memcmp| replace:: similar to C 'memcmp', compares `size` bytes of `left`` and `right` memory. returns -1 if left is less, 1 if left is greater, and 0 if left is same as right

.. |function-builtin-panic| replace:: will cause panic. The program will be determinated if there is no recover. Panic is not a error handling mechanism and can not be used as such. It is indeed panic, fatal error. It is not supposed that program can completely correctly recover from panic, recover construction is provided so program can try to correcly shut-down or report fatal error. If there is no recover withing script, it will be called in calling eval (in C++ callee code).

.. |function-builtin-peek| replace:: returns contents of the das::string as temporary string value. this is fastest way to access contents of das::string as string

.. |function-builtin-print| replace:: outputs string into current context log output

.. |function-builtin-profile| replace:: profiles specified block by evaluating it `count` times and returns minimal time spent in the block in seconds, as well as prints it.

.. |function-builtin-reset_profiler| replace:: resets counters in the built-in profiler

.. |function-builtin-set| replace:: to be documented

.. |function-builtin-set_variant_index| replace:: sets internal index of the variant value

.. |function-builtin-smart_ptr_clone| replace:: clones smart_ptr, internal use-count is incremented

.. |function-builtin-smart_ptr_use_count| replace:: returns internal use-count for the smart_ptr

.. |function-builtin-sprint| replace:: similar to 'print' but returns string instead of printing it

.. |function-builtin-sprint_json| replace:: similar to 'write_json' but skips intermediate representation. this is faster but less flexible

.. |function-builtin-stackwalk| replace:: stackwalk prints call stack and local variables values

.. |function-builtin-string_heap_bytes_allocated| replace:: returns number of bytes allocated in the string heap

.. |function-builtin-string_heap_collect| replace:: calls garbage collection on the string heap

.. |function-builtin-string_heap_depth| replace:: returns number of generations in the string heap

.. |function-builtin-string_heap_report| replace:: reports string heap usage and allocations

.. |function-builtin-terminate| replace:: terminates current context execution

.. |function-builtin-variant_index| replace:: returns internal index of the variant value

.. |function-builtin-binary_load| replace:: loads any data from array<uint8>. obsolete, use daslib/archive instead

.. |function-builtin-binary_save| replace:: saves any data to array<uint8>. obsolete, use daslib/archive instead

.. |function-builtin-clone_dim| replace:: to be documented

.. |function-builtin-clone_to_move| replace:: to be documented

.. |function-builtin-each| replace:: returns iterator, which iterates though each element of the object. object can be range, static or dynamic array, another iterator.

.. |function-builtin-each_enum| replace:: iterates over each element in the enumeration

.. |function-builtin-each_ref| replace:: similar to each, but iterator returns references instead of values

.. |function-builtin-emplace| replace:: emplace will push to dynamic array `array_arg` the content of `value`. `value` has to be of the same type (or const reference to same type) as array values. if `at` is provided `value` will be pushed at index `at`, otherwise to the end of array. The `content` of value will be moved (<-) to it.

.. |function-builtin-erase| replace:: erase will erase `at` index element in `arg` array.

.. |function-builtin-finalize| replace:: to be documented

.. |function-builtin-finalize_dim| replace:: to be documented

.. |function-builtin-get| replace:: will execute `block_arg` with argument reference-to-value in `table_arg` referencing value indexed by `key`. Will return false if `key` doesn't exist in `table_arg`, otherwise true.

.. |function-builtin-find| replace:: will execute `block_arg` with argument pointer-to-value in `table_arg` pointing to value indexed by `key`, or null if `key` doesn't exist in `table_arg`.

.. |function-builtin-find_for_edit| replace:: similar to find, but pointer to the value would be read-write

.. |function-builtin-find_for_edit_if_exists| replace:: similar to find_if_exists, but pointer to the value would be read-write

.. |function-builtin-find_if_exists| replace:: similar to find, but the block will only be called, if the key is found

.. |function-builtin-find_index| replace:: returns index of they key in the array

.. |function-builtin-find_index_if| replace:: returns index of the key in the array, where key is checked via compare block

.. |function-builtin-get_ptr| replace:: returns regular pointer from the smart_ptr

.. |function-builtin-has_value| replace:: returns true if iterable `a` (array, dim, etc) contains `key`

.. |function-builtin-intptr| replace:: returns int64 representation of a pointer

.. |function-builtin-key_exists| replace:: will return true if element `key` exists in table `table_arg`.

.. |function-builtin-keys| replace:: returns iterator to all keys of the table

.. |function-builtin-lock| replace:: locks array or table for the duration of the block invocation, so that it can't be resized. values can't be pushed or popped, etc.

.. |function-builtin-lock_forever| replace:: locks array or table forever

.. |function-builtin-next| replace:: returns next element in the iterator as the 'value'. result is true if there is element returned, or false if iterator is null or empty

.. |function-builtin-nothing| replace:: returns empty iterator

.. |function-builtin-pop| replace:: removes last element of the array

.. |function-builtin-push| replace:: push will push to dynamic array `array_arg` the content of `value`. `value` has to be of the same type (or const reference to same type) as array values. if `at` is provided `value` will be pushed at index `at`, otherwise to the end of array. The `content` of value will be copied (assigned) to it.

.. |function-builtin-push_clone| replace:: similar to `push`, only values would be cloned instead of copied

.. |function-builtin-reserve| replace:: makes sure array has sufficient amount of memory to hold specified number of elements. reserving arrays will speed up pushing to it

.. |function-builtin-resize| replace:: Resize will resize `array_arg` array to a new size of `new_size`. If new_size is bigger than current, new elements will be zeroed.

.. |function-builtin-resize_no_init| replace:: Resize will resize `array_arg` array to a new size of `new_size`. If new_size is bigger than current, new elements will be left uninitialized.

.. |function-builtin-sort| replace:: sorts an array in ascending order.

.. |function-builtin-to_array| replace:: will convert argument (static array, iterator, another dynamic array) to an array. argument elements will be cloned

.. |function-builtin-to_array_move| replace:: will convert argument (static array, iterator, another dynamic array) to an array. argument elements will be copied or moved

.. |function-builtin-to_table| replace:: will convert an array of key-value tuples into a table<key;value> type. arguments will be cloned

.. |function-builtin-to_table_move| replace:: will convert an array of key-value tuples into a table<key;value> type. arguments will be copied or moved

.. |function-builtin-values| replace:: returns iterator to all values of the table

.. |any_annotation-builtin-clock| replace:: das::Time which is a wrapper around `time_t`

.. |any_annotation-builtin-das_string| replace:: das::string which is typically std::string or equivalent

.. |variable-builtin-DBL_MAX| replace:: maximum possible value of 'double'

.. |variable-builtin-DBL_MIN| replace:: smallest possible non-zero value of 'double'. if u want minimum possible value use `-DBL_MAX`

.. |variable-builtin-FLT_MAX| replace:: maximum possible value of 'float'

.. |variable-builtin-FLT_MIN| replace:: smallest possible non-zero value of 'float'. if u want minimum possible value use `-FLT_MAX`

.. |variable-builtin-INT_MAX| replace:: maximum possible value of 'int'

.. |variable-builtin-INT_MIN| replace:: minimum possible value of 'int'

.. |variable-builtin-LONG_MAX| replace:: maximum possible value of 'int64'

.. |variable-builtin-LONG_MIN| replace:: minimum possible value of 'int64'

.. |variable-builtin-UINT_MAX| replace:: maximum possible value of 'uint'

.. |variable-builtin-ULONG_MAX| replace:: minimum possible value of 'uint64'

.. |variable-builtin-LOG_CRITICAL| replace:: indicates maximum log level. critial errors, panic, shutdown

.. |variable-builtin-LOG_ERROR| replace:: indicates log level recoverable errors

.. |variable-builtin-LOG_WARNING| replace:: indicates log level for API misuse, non-fatal errors

.. |variable-builtin-LOG_INFO| replace:: indicates log level for miscellaneous informative messages

.. |variable-builtin-LOG_DEBUG| replace:: indicates log level for debug messages

.. |variable-builtin-LOG_TRACE| replace:: indicates log level for the most noisy debug and tracing messages

.. |variable-builtin-print_flags_debugger| replace:: printing flags similar to those used by the 'debug' function

.. |function_annotation-builtin-deprecated| replace:: deprecated annotation is used to mark a function as deprecated. it will generate a warning during compilation, and will not be callable from the final compiled context

.. |function_annotation-builtin-alias_cmres| replace:: indicates that function always aliases cmres (copy or move result), and cmres optimizations are disabled.

.. |function_annotation-builtin-never_alias_cmres| replace:: indicates that function never aliases cmres (copy or move result), and cmres checks will not be performed

.. |function_annotation-builtin-marker| replace:: marker annotation is used to attach arbitrary marker values to a function (in form of annotation arguments). its typically used for implementation of macros

.. |function_annotation-builtin-generic| replace:: indicates that the function is generic, regardless of its argument types. generic functions will be instanced in the calling module

.. |function_annotation-builtin-_macro| replace:: indicates that the function will be called during the macro pass, similar to `[init]`

.. |function_annotation-builtin-macro_function| replace:: indicates that the function is part of the macro implementation, and will not be present in the final compiled context, unless explicitly called.

.. |function_annotation-builtin-export| replace:: indicates that function is to be exported to the final compiled context

.. |function_annotation-builtin-no_lint| replace:: indicates that the lint pass should be skipped for the specific function

.. |function_annotation-builtin-sideeffects| replace:: indicates that the function should be treated as if it has side-effects. for example it will not be optimized out

.. |function_annotation-builtin-run| replace:: ensures that the function is always evaluated at compilation time

.. |function_annotation-builtin-unsafe_operation| replace:: indicates that function is unsafe, and will require `unsafe` keyword to be called

.. |function_annotation-builtin-no_aot| replace:: indicates that the AOT will not be generated for this specific function

.. |function_annotation-builtin-init| replace:: indicates that the function would be called at the context initialization time

.. |function_annotation-builtin-finalize| replace:: indicates that the function would be called at the context shutdown time

.. |function_annotation-builtin-hybrid| replace:: indicates that the function is likely candidate for later patching, and the AOT will generate hybrid calls to it - instead of direct calls. that way modyfing the function will not affect AOT of other functions.

.. |function_annotation-builtin-unsafe_deref| replace:: optimization, which indicates that pointer dereference, array and string indexing, and few other operations would not check for null or bounds

.. |function_annotation-builtin-unused_argument| replace:: marks function arguments, which are unused. that way when code policies make unused arguments an error, a workaround can be provided

.. |function_annotation-builtin-local_only| replace:: indicates that function can only accept local `make` expressions, like [[make tuple]] and [[make structure]]

.. |function_annotation-builtin-expect_any_vector| replace:: indicates that function can only accept das::vector templates

.. |function_annotation-builtin-builtin_array_sort| replace:: indicates sort function for builtin 'sort' machinery. used internally

.. |function_annotation-builtin-concept_assert| replace:: similar to regular `assert` function, but always happens at compilation time. it would also display the error message from where the asserted function was called from, not the assert line itself.

.. |function_annotation-builtin-__builtin_table_key_exists| replace:: part of internal implementation for `key_exists`

.. |function_annotation-builtin-static_assert| replace:: similar to regular `assert` function, but always happens at compilation time

.. |function_annotation-builtin-verify| replace:: assert for the expression with side effects. expression will not be optimized out if asserts are disabled

.. |function_annotation-builtin-debug| replace:: prints value and returns that same value

.. |function_annotation-builtin-assert| replace:: throws panic if first operand is false. can be disabled. second operand is error message

.. |function_annotation-builtin-memzero| replace:: initializes section of memory with '0'

.. |function_annotation-builtin-__builtin_table_find| replace:: part of internal implementation for `find`

.. |function_annotation-builtin-invoke| replace:: invokes block, function, or lambda

.. |function_annotation-builtin-__builtin_table_erase| replace:: part of internal implementation for `erase`

.. |function-builtin-is_in_aot| replace:: returns true if compiler is currently generating AOT

.. |function-builtin-memcpy| replace:: copies `size` bytes of memory from `right` to `left`

.. |function-builtin-lock_data| replace:: locks array and invokes block with a pointer to array's data

.. |function-builtin-map_to_array| replace:: builds temporary array from the specified memory

.. |function-builtin-map_to_ro_array| replace:: same as `map_to_array` but array is read-only

.. |function-builtin-ref_time_ticks| replace:: returns current time in ticks

.. |function-builtin-get_time_usec| replace:: returns time interval in usec, since the specified `reft` (usually from `ref_time_ticks`)

.. |function-builtin-get_time_nsec| replace:: returns time interval in nsec, since the specified `reft` (usually from `ref_time_ticks`)

.. |function-builtin-iter_range| replace:: returns range(`foo`)

.. |function-builtin-swap| replace:: swaps two values `a` and 'b'

.. |function-builtin-interval| replace:: returns range('arg0','arg1')

.. |function-builtin-lock_count| replace:: returns internal lock count for the array or table

.. |function-builtin-error| replace:: similar to 'print' but outputs to context error output

.. |function-builtin-memory_report| replace:: reports memory allocation, optionally GC errors only

.. |function-builtin-class_rtti_size| replace:: returns size of specific TypeInfo for the class

.. |function-builtin-to_log| replace:: similar to print but output goes to the logging infrastructure. `arg0` specifies log level, i.e. LOG_... constants

.. |function-builtin-_move_with_lockcheck| replace:: moves `b` into `a`, checks if `a` or `b` is locked, recursively for each lockable element of a and b

.. |function-builtin-_return_with_lockcheck| replace:: returns `a`. check if `a` is locked, recursively for each lockable element of a

.. |function-builtin-back| replace:: returns last element of the array

.. |function-builtin-_at_with_lockcheck| replace:: returns element of the table `Tab`, also checks if `Tab` is locked, recursively for each lockable element of `Tab`

.. |function-builtin-get_const_ptr| replace:: return constant pointer from regular pointer

.. |function-builtin-subarray| replace:: returns new array which is copy of a slice of range of the source array

.. |function-builtin-move_to_ref| replace:: moves `b` into `a`. if `b` is value, it will be copied to `a` instead

.. |function-builtin-copy_to_local| replace:: copies value and returns it as local value on stack. used to work around aliasing issues

.. |function-builtin-move_to_local| replace:: moves value and returns it as local value on stack. used to work around aliasing issues

.. |reader_macro-builtin-_esc| replace:: returns raw string input, without regards for escape sequences. For example %_esc\\n\\r%_esc will return 4 character string '\\','n','\\','r'

.. |typeinfo_macro-builtin-rtti_classinfo| replace:: Generates TypeInfo for the class initialization.

.. |variable-builtin-VEC_SEP| replace:: Read-only string constant which is used to separate elements of vectors. By default its ","

.. |function_annotation-builtin-__builtin_table_set_insert| replace:: part of internal implementation for `insert` of the sets (tables with keys only).

.. |function-builtin-is_in_completion| replace:: returns true if compiler is currently generating completion, i.e. lexical representation of the program for the text editor's text completion system.

.. |function-builtin-insert| replace:: inserts key into the set (table with no values) `Tab`

.. |function-builtin-add_ptr_ref| replace:: increases reference count of the smart pointer.

.. |function-builtin-clz| replace:: count leading zeros

.. |function-builtin-ctz| replace:: count trailing zeros

.. |function-builtin-popcnt| replace:: count number of set bits

.. |structure_macro-builtin-comment| replace:: [comment] macro, which does absolutely nothing but holds arguments.

.. |structure_macro-builtin-macro_interface| replace:: [macro_interface] specifies that class and its inherited children are used as a macro interfaces, and would not be exported by default.

.. |structure_macro-builtin-cpp_layout| replace:: [cpp_layout] specifies that structure uses C++ memory layout rules, as oppose to native daScript memory layout rules.

.. |structure_macro-builtin-persistent| replace:: [persistent] annotation specifies that structure is allocated (via new) on the C++ heap, as oppose to daScript context heap.

.. |variable-builtin-SIZE_OF_PROLOGUE| replace:: size of the Prologue structure on the call stack.

.. |variable-builtin-CONTEXT_OFFSET_OF_EVAL_TOP| replace:: offset of the eval stack top pointer in the `Context`.

.. |variable-builtin-CONTEXT_OFFSET_OF_GLOBALS| replace:: offset of the global variables in the `Context`.

.. |function_annotation-builtin-jit| replace:: Explicitly marks (forces) function to be compiled with JIT compiler.

.. |function_annotation-builtin-unsafe_outside_of_for| replace:: Marks function as unsafe to be called outside of the sources `for` loop.

.. |function_annotation-builtin-skip_lock_check| replace:: optimization, which indicates that lock checks are not needed in this function.

.. |structure_macro-builtin-skip_field_lock_check| replace:: optimization, which indicates that the structure does not need lock checks.

.. |function-builtin-set_verify_array_locks| replace:: runtime optimization, which indicates that the array does not need lock checks.

.. |function-builtin-set_verify_table_locks| replace:: runtime optimization, which indicates that the table does not need lock checks.

.. |function-builtin-invoke_code| replace:: Invokes function, declared as `vec4f(*)(Context *, vec4f *, void *)`.

.. |function-builtin-instrument_jit| replace:: Replaces function simulation node with 'SimNode_Jit', which will call function pointer directly. Function pointer format is `vec4f(*)(Context *, vec4f *, void *)`

.. |function-builtin-remove_jit| replace:: Restores original function simulation nodes.

.. |function-builtin-is_jit_function| replace:: Returns true if function is instrumented with JIT.

.. |function-builtin-get_jit_exception| replace:: Returns pointer to `void jit_exception ( const char * text, Context * context )`

.. |function-builtin-get_jit_call_or_fastcall| replace:: Returns pointer to `vec4f jit_call_or_fastcall ( SimFunction * fn, vec4f * args, Context * context )`

.. |function-builtin-get_jit_call_with_cmres| replace:: Returns pointer to `vec4f jit_call_with_cmres ( SimFunction * fn, vec4f * args, void * cmres, Context * context )`

.. |function-builtin-get_jit_invoke_block| replace:: Returns pointer to `vec4f jit_invoke_block ( const Block & blk, vec4f * args, Context * context )`

.. |function-builtin-get_jit_invoke_block_with_cmres| replace:: Returns pointer to `vec4f jit_invoke_block_with_cmres ( const Block & blk, vec4f * args, void * cmres, Context * context )`

.. |function-builtin-get_jit_string_builder| replace:: Returns pointer to `char * jit_string_builder ( Context & context, SimNode_CallBase * call, vec4f * args )`

.. |function-builtin-get_jit_get_global_mnh| replace:: Returns pointer to `void * jit_get_global_mnh ( uint64_t mnh, Context & context )`

.. |function-builtin-get_jit_alloc_heap| replace:: Returns pointer to `void * jit_alloc_heap ( uint32_t bytes, Context * context )`

.. |function-builtin-get_jit_alloc_persistent| replace:: Returns pointer to `void * jit_alloc_persistent ( uint32_t bytes, Context * )`

.. |function-builtin-get_jit_free_heap| replace:: Returns pointer to `void jit_free_heap ( void * bytes, uint32_t size, Context * context )`

.. |function-builtin-get_jit_free_persistent| replace:: Returns pointer to `void jit_free_persistent ( void * bytes, Context * )`

.. |function-builtin-get_jit_array_lock| replace:: Returns pointer to `void builtin_array_lock ( const Array & arr, Context * context )`

.. |function-builtin-get_jit_array_unlock| replace:: Returns pointer to `void builtin_array_unlock ( const Array & arr, Context * context )`

.. |function-builtin-get_jit_table_at| replace:: Returns pointer to `template <typename KeyType> int32_t jit_table_at ( Table * tab, KeyType key, int32_t valueTypeSize, Context * context )`

.. |function-builtin-get_jit_str_cmp| replace:: Returns pointer to `int32_t jit_str_cmp ( char * a, char * b )`

.. |function-builtin-get_jit_prologue| replace:: Returns pointer to `void jit_prologue ( int32_t stackSize, JitStackState * stackState, Context * context )`

.. |function-builtin-get_jit_epilogue| replace:: Returns pointer to `void jit_epilogue ( JitStackState * stackState, Context * context )`

.. |function-builtin-get_jit_make_block| replace:: Returns pointer to `void jit_make_block ( Block * blk, int32_t argStackTop, void * bodyNode, void * funcInfo, Context * context )`

.. |function-builtin-using| replace:: Creates temporary das_string.

.. |function_annotation-builtin-hint| replace:: Hints the compiler to use specific optimization.

.. |function-builtin-set_verify_context_locks| replace:: Enables or disables array or table lock runtime verification per context

.. |function-builtin-count| replace:: returns iterator which iterates from `start` value by incrementing it by `step` value. It is the intended way to have counter together with other values in the `for` loop.

.. |function-builtin-ucount| replace:: returns iterator which iterates from `start` value by incrementing it by `step` value. It is the intended way to have counter together with other values in the `for` loop.

.. |function-builtin-move_new| replace:: Moves the new [[...]] value into smart_ptr.

.. |function-builtin-move| replace:: Moves one smart_ptr into another. Semantic equivalent of move(a,b) => a := null, a <- b

.. |function-builtin-to_compiler_log| replace:: Output text to compiler log, usually from the macro.

.. |function-builtin-memset8| replace:: Effecitvely C memset.

.. |function-builtin-memset16| replace:: Similar to memset, but fills values with 16 bit words.

.. |function-builtin-memset32| replace:: Similar to memset, but fills values with 32 bit words.

.. |function-builtin-memset64| replace:: Similar to memset, but fills values with 64 bit words.

.. |function-builtin-memset128| replace:: Similar to memset, but fills values with 128 bit vector type values.


