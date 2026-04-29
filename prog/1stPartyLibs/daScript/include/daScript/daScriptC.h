#pragma once

#include <stdint.h>

#if DAS_ENABLE_DLL
#ifndef DAS_API
#ifdef _MSC_VER
    #define DAS_EXPORT_DLL __declspec(dllexport)
    #define DAS_IMPORT_DLL __declspec(dllimport)
#else
    #define DAS_EXPORT_DLL __attribute__((visibility("default")))
    #define DAS_IMPORT_DLL
#endif
#ifdef DAS_EXPORTS
    #define DAS_API DAS_EXPORT_DLL
#else
    #define DAS_API DAS_IMPORT_DLL
#endif
#endif
#else
#define DAS_API
#endif
//if target is not defined, try to auto-detect target
#ifndef _TARGET_SIMD_SSE
    #if __SSE4_1__ || defined(__AVX__) || defined(__AVX2__)
        #define _TARGET_SIMD_SSE 4
    #elif __SSSE3__
        #define _TARGET_SIMD_SSE 3
    #elif defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP>=1)
        #define _TARGET_SIMD_SSE 2
    #endif
#endif

#if !defined(_TARGET_SIMD_SSE) && !defined(_TARGET_SIMD_NEON)
    #if defined(__ARM_NEON) || defined(__ARM_NEON__)
        #define _TARGET_SIMD_NEON 1
    #endif
#endif

#if defined(_TARGET_SIMD_SSE)
    #include <emmintrin.h>
    typedef __m128 vec4f;
    typedef __m128i vec4i;
#elif defined(_TARGET_SIMD_NEON) // PSP2, iOS
    #include <arm_neon.h>
    typedef float32x4_t vec4f;
    typedef int32x4_t   vec4i;
#else
    #error unsupported target
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern DAS_API uint32_t SIDEEFFECTS_none;
extern DAS_API uint32_t SIDEEFFECTS_unsafe;
extern DAS_API uint32_t SIDEEFFECTS_userScenario;
extern DAS_API uint32_t SIDEEFFECTS_modifyExternal;
extern DAS_API uint32_t SIDEEFFECTS_accessExternal;
extern DAS_API uint32_t SIDEEFFECTS_modifyArgument;
extern DAS_API uint32_t SIDEEFFECTS_modifyArgumentAndExternal;
extern DAS_API uint32_t SIDEEFFECTS_worstDefault;
extern DAS_API uint32_t SIDEEFFECTS_accessGlobal;
extern DAS_API uint32_t SIDEEFFECTS_invoke;
extern DAS_API uint32_t SIDEEFFECTS_inferredSideEffects;

typedef struct dasTextOutputHandle das_text_writer;
typedef struct dasModuleGroupHandle das_module_group;
typedef struct dasFileAccessHandle das_file_access;
typedef struct dasProgram das_program;
typedef struct dasContext das_context;
typedef struct dasFunction das_function;
typedef struct dasError das_error;
typedef struct dasModule das_module;
typedef struct dasNode das_node;
typedef struct dasStructure das_structure;
typedef struct dasEnumeration das_enumeration;
typedef struct dasLambda das_lambda;
typedef struct dasBlock das_block;
typedef struct dasPolicies das_policies;
typedef struct dasSerializedData das_serialized_data;
typedef struct dasEnvironmentHandle das_environment;
typedef struct dasReuseCacheGuardHandle das_reuse_cache_guard;
typedef struct dasTypeInfoHandle das_type_info;
typedef struct dasStructInfoHandle das_struct_info;
typedef struct dasEnumInfoHandle das_enum_info;
typedef struct dasFuncInfoHandle das_func_info;

typedef struct {
    float x, y, z, w;
} vec4f_unaligned;

typedef struct {
    char *      data;
    uint32_t    size;
    uint32_t    capacity;
    uint32_t    lock;
    uint32_t    flags;
} das_array;

typedef struct {
    char *      data;
    uint32_t    size;
    uint32_t    capacity;
    uint32_t    lock;
    uint32_t    flags;
    char *      keys;
    uint32_t *  hashes;
    uint32_t    tombstones;
} das_table;

typedef vec4f (das_interop_function) ( das_context * ctx, das_node * node, vec4f * arguments );

typedef void (das_interop_function_unaligned) ( das_context * ctx, das_node * node, vec4f_unaligned * arguments, vec4f_unaligned * result );

// --- Lifecycle ---

// Initialize all built-in modules and internal structures.
// Must be called once before any other daslang C API call.
DAS_API void das_initialize();
// Shut down the daslang runtime and free all internal structures and memory.
// Must be called once when the host application is done with daslang.
DAS_API void das_shutdown();

// --- Text output ---

// Create a text writer that prints to stdout.
// Use das_text_make_writer() instead if you need to capture output as a string.
DAS_API das_text_writer * das_text_make_printer();
// Create a text writer that accumulates output in an internal string buffer.
// Use das_text_make_printer() instead if you just want stdout output.
DAS_API das_text_writer * das_text_make_writer();
// Release a text writer created by das_text_make_printer() or das_text_make_writer().
DAS_API void das_text_release ( das_text_writer * output );
// Write a null-terminated string to a text writer.
DAS_API void das_text_output ( das_text_writer * output, char * text );

// --- Module groups ---

// Create a module group. A module group holds references to modules
// that the compiler should use when resolving `require` statements.
DAS_API das_module_group * das_modulegroup_make ();
// Add a module to a module group so that compiled scripts can `require` it.
DAS_API void das_modulegroup_add_module ( das_module_group* lib, das_module* mod );
// Release a module group created by das_modulegroup_make().
DAS_API void das_modulegroup_release ( das_module_group * group );

// --- Dynamic modules ---

// Scan <project_root>/modules/ for .das_module descriptor files and
// call `initialize` on each one.
//
// If tout is NULL, diagnostic output goes to stdout.
//
// Returns 0 on success.
DAS_API int das_register_dynamic_modules(das_file_access *file_access,
                                         const char *project_root,
                                         das_text_writer *tout);

// --- File access ---

// Create a default file access that reads directly from the filesystem.
DAS_API das_file_access * das_fileaccess_make_default (  );
// Create a file access backed by a .das_project file.
// The project file is a daslang program that exports callback functions
// controlling module resolution, permissions, and compilation policies.
// See tutorial 06 (sandbox) for a complete example.
DAS_API das_file_access * das_fileaccess_make_project ( const char * project_file  );
// Release a file access created by das_fileaccess_make_default() or das_fileaccess_make_project().
DAS_API void das_fileaccess_release ( das_file_access * access );
// Register a virtual file from a C string.
// The file appears at 'file_name' during compilation even though it does not exist on disk.
// If 'owns' is non-zero, the content is copied internally and the caller may free file_content.
// If 'owns' is zero, the content is borrowed and the caller must keep file_content alive
// for the lifetime of the file access.
DAS_API void das_fileaccess_introduce_file ( das_file_access * access, const char * file_name, const char * file_content, int owns );

// Read a file from disk at 'disk_path' and cache it under the virtual path 'name'.
// 'name' is what module resolution returns (e.g. "daslib/strings.das");
// 'disk_path' is the actual location on the filesystem.
DAS_API void das_fileaccess_introduce_file_from_disk ( das_file_access * access, const char * name, const char * disk_path );
// Pre-load all standard library modules from getDasRoot()/daslib/ into the file access cache.
// Typically called before das_fileaccess_lock() when setting up a sandbox.
DAS_API void das_fileaccess_introduce_daslib ( das_file_access * access );
// Pre-load all native plugin modules listed in external_resolve.inc into the file access cache.
// Modules not present on disk are silently skipped.
DAS_API void das_fileaccess_introduce_native_modules ( das_file_access * access );
// Pre-load a single native module by its require-style path (e.g. "opengl/opengl_boost").
// Returns 1 if the module file was found and cached, 0 otherwise.
DAS_API int das_fileaccess_introduce_native_module ( das_file_access * access, const char * req );

// Lock the file access. While locked, getNewFileInfo() returns NULL for all
// files not already in the cache, so only pre-introduced files can be accessed.
DAS_API void das_fileaccess_lock ( das_file_access * access );
// Unlock the file access, re-enabling filesystem reads and further introduce calls.
DAS_API void das_fileaccess_unlock ( das_file_access * access );
// Return 1 if the file access is locked, 0 otherwise.
DAS_API int das_fileaccess_is_locked ( das_file_access * access );

// Copy the daslang root path into 'root'. At most 'maxbuf' bytes are written
// (including the null terminator). The root path is used to locate daslib/ and other resources.
DAS_API void das_get_root ( char * root, int maxbuf );

// --- Compilation ---

// Compile a daslang program from the file at 'program_file'.
// Compiler diagnostics are written to 'tout'. The returned program is always
// non-NULL; check das_program_err_count() to determine if compilation succeeded.
DAS_API das_program * das_program_compile ( char * program_file, das_file_access * access, das_text_writer * tout, das_module_group * libgroup );
// Release a compiled program.
DAS_API void das_program_release ( das_program * program );
// Return the number of compilation errors. Zero means the program compiled successfully.
DAS_API int das_program_err_count ( das_program * program );
// Return the minimum context stack size (in bytes) needed to simulate this program.
// Pass this value to das_context_make().
DAS_API int das_program_context_stack_size ( das_program * program );
// Simulate (link) a compiled program into a context.
// Returns 1 on success, 0 on failure. On failure, error details can be retrieved
// with das_program_get_error(). The text writer 'tout' receives diagnostic output.
DAS_API int das_program_simulate ( das_program * program, das_context * ctx, das_text_writer * tout );
// Return the i-th compilation or simulation error, or NULL if index is out of range.
// Valid indices are [0, das_program_err_count(program)).
DAS_API das_error * das_program_get_error ( das_program * program, int index );

// --- Errors ---

// Write a human-readable error description to a text writer.
DAS_API void das_error_output ( das_error * error, das_text_writer * tout );
// Write a human-readable error description into 'text' (at most 'maxLength' bytes,
// including the null terminator). Truncated if the message exceeds maxLength.
DAS_API void das_error_report ( das_error * error, char * text, int maxLength );

// --- Context ---

// Create an execution context with the given stack size (in bytes).
// stackSize must be >= das_program_context_stack_size(program) for the program
// you intend to simulate.
DAS_API das_context * das_context_make ( int stackSize );
// Release an execution context.
DAS_API void das_context_release ( das_context * context );
// Find an exported function by name in a simulated context.
// Returns NULL if no function with that name exists.
DAS_API das_function * das_context_find_function ( das_context * context, char * name );

// --- Threading: environment ---
// daScriptEnvironment is thread-local (TLS).  Every thread that touches
// the daslang API must have a valid environment bound.  Use getBound/setBound
// to share the main thread's environment with a worker that only executes
// (Part A pattern), or call das_initialize()/das_shutdown() on the worker
// for a fully independent environment (Part B pattern).

// Return the environment bound to the current thread, or NULL.
DAS_API das_environment * das_environment_get_bound ();
// Bind an environment on the current thread.  The worker thread must call
// this before any other daslang API call when sharing the main thread's env.
DAS_API void das_environment_set_bound ( das_environment * env );

// --- Threading: reuse cache guard ---
// A thread-local free-list cache used by the daslang allocator.
// Must be created first on any worker thread that uses daslang.

// Create a reuse-cache guard (call at thread start).
DAS_API das_reuse_cache_guard * das_reuse_cache_guard_create ();
// Release a reuse-cache guard (call before thread exit).
DAS_API void das_reuse_cache_guard_release ( das_reuse_cache_guard * guard );

// --- Threading: context cloning ---
// Clone a context for use on a worker thread.  The clone has its own
// stack and heap but shares the simulated program.  Release with
// das_context_release().

// Context-category constants (bitmask flags).
extern DAS_API uint32_t DAS_CONTEXT_CATEGORY_THREAD_CLONE;
extern DAS_API uint32_t DAS_CONTEXT_CATEGORY_JOB_CLONE;

// Clone 'source' with the given category flags.  Typically pass
// DAS_CONTEXT_CATEGORY_THREAD_CLONE for threading.
DAS_API das_context * das_context_clone ( das_context * source, uint32_t category );

// --- Function evaluation (aligned, vec4f) ---
// All arguments and return values are passed as vec4f (16-byte aligned SIMD registers).
// Use the das_argument_* helpers to pack arguments and das_result_* to unpack results.
// If the function throws an exception, retrieve it with das_context_get_exception().

// Call a function. Returns the result as vec4f.
DAS_API vec4f das_context_eval_with_catch ( das_context * context, das_function * fun, vec4f * arguments );

// --- Function evaluation (unaligned, vec4f_unaligned) ---
// Same semantics as above, but arguments and result use vec4f_unaligned (no alignment requirement).
// Prefer these when calling from plain C code that cannot guarantee 16-byte alignment.
// 'narguments' is the number of elements in the arguments array.

// Call a function with unaligned arguments. Result is written into *result.
DAS_API void das_context_eval_with_catch_unaligned ( das_context * context, das_function * fun, vec4f_unaligned * arguments, int narguments, vec4f_unaligned * result );

// --- Function evaluation returning complex results (cmres) ---
// Functions that return structures, tuples, or other types that do not fit in a single
// register use a caller-allocated buffer (cmres). The caller must allocate enough memory
// for the return type and pass it via the 'cmres' pointer.

// Call a function that returns a complex result (aligned arguments).
DAS_API void das_context_eval_with_catch_cmres ( das_context * context, das_function * fun, vec4f * arguments, void * cmres );
// Call a function that returns a complex result (unaligned arguments).
DAS_API void das_context_eval_with_catch_cmres_unaligned ( das_context * context, das_function * fun, vec4f_unaligned * arguments, int narguments, void * cmres );
// Return the current exception message, or NULL if no exception occurred.
DAS_API char * das_context_get_exception ( das_context * context );

// --- Lambda evaluation ---
// Lambdas are heap-allocated closures. The same aligned / unaligned / cmres
// variants apply as for function evaluation above.

// Call a lambda (aligned). Returns result as vec4f.
DAS_API vec4f das_context_eval_lambda ( das_context * context, das_lambda * lambda, vec4f * arguments );
// Call a lambda (unaligned). Result written into *result.
DAS_API void das_context_eval_lambda_unaligned ( das_context * context, das_lambda * lambda, vec4f_unaligned * arguments, int narguments, vec4f_unaligned * result );
// Call a lambda returning a complex result (aligned).
DAS_API void das_context_eval_lambda_cmres ( das_context * context, das_lambda * lambda, vec4f * arguments, void * cmres );
// Call a lambda returning a complex result (unaligned).
DAS_API void das_context_eval_lambda_cmres_unaligned ( das_context * context, das_lambda * lambda, vec4f_unaligned * arguments, int narguments, void * cmres );

// --- Block evaluation ---
// Blocks are stack-allocated closures. The same aligned / unaligned / cmres
// variants apply as for function evaluation above.

// Call a block (aligned). Returns result as vec4f.
DAS_API vec4f das_context_eval_block ( das_context * context, das_block * block, vec4f * arguments );
// Call a block (unaligned). Result written into *result.
DAS_API void das_context_eval_block_unaligned ( das_context * context, das_block * block, vec4f_unaligned * arguments, int narguments, vec4f_unaligned * result );
// Call a block returning a complex result (aligned).
DAS_API void das_context_eval_block_cmres ( das_context * context, das_block * block, vec4f * arguments, void * cmres );
// Call a block returning a complex result (unaligned).
DAS_API void das_context_eval_block_cmres_unaligned ( das_context * context, das_block * block, vec4f_unaligned * arguments, int narguments, void * cmres );

// --- Type binding: structures ---

// Create a handled structure type that mirrors a C struct.
// 'name' is the daslang name, 'cppname' is the C/C++ type name used for mangling,
// 'sz' and 'al' are sizeof() and alignof() of the C struct.
// The returned handle must be populated with das_structure_add_field() and then
// registered into a module with das_module_bind_structure().
DAS_API das_structure * das_structure_make ( das_module_group * lib, const char * name, const char * cppname, int sz, int al );
// Add a field to a structure created by das_structure_make().
// 'offset' is offsetof() of the field in the C struct.
// 'tname' is the field type in mangled-name format (see type_mangling.rst).
DAS_API void das_structure_add_field ( das_structure * st, das_module * mod, das_module_group * lib,  const char * name, const char * cppname, int offset, const char * tname );

// --- Type binding: enumerations ---

// Create an enumeration type.
// 'name' is the daslang name, 'cppname' is the C/C++ type name.
// 'ext' is non-zero if the enum's underlying storage is int64 (otherwise int).
// Populate with das_enumeration_add_value(), then register with das_module_bind_enumeration().
DAS_API das_enumeration * das_enumeration_make ( const char * name, const char * cppname, int ext );
// Add a named value to an enumeration created by das_enumeration_make().
DAS_API void das_enumeration_add_value ( das_enumeration * enu, const char * name, const char * cppName, int value );

// --- Modules ---

// Create a new module with the given name.
// After populating it with bindings, add it to a module group with das_modulegroup_add_module().
DAS_API das_module * das_module_create ( char * name );
// Find an already-registered module by name (e.g. "rtti_core", "$").
// Returns NULL if no module with that name exists.
DAS_API das_module * das_module_find ( const char * name );
// Bind a C interop function (aligned, vec4f calling convention) to a module.
// 'name' is the daslang function name, 'cppName' is the C symbol name.
// 'sideffects' is a combination of SIDEEFFECTS_* flags.
// 'args' is a mangled signature string describing argument and return types
// (see type_mangling.rst).
DAS_API void das_module_bind_interop_function ( das_module * mod, das_module_group * lib, das_interop_function * fun, char * name, char * cppName, uint32_t sideffects, char* args );
// Bind a C interop function (unaligned, vec4f_unaligned calling convention) to a module.
// Same parameters as das_module_bind_interop_function(); use this variant when the
// interop function uses vec4f_unaligned arguments and result.
DAS_API void das_module_bind_interop_function_unaligned ( das_module * mod, das_module_group * lib, das_interop_function_unaligned * fun, char * name, char * cppName, uint32_t sideffects, char* args );
// Bind a type alias to a module. 'aname' is the alias name visible in daslang,
// 'tname' is the target type in mangled-name format.
DAS_API void das_module_bind_alias ( das_module * mod, das_module_group * lib, char * aname, char * tname );
// Register a structure (from das_structure_make()) into a module.
DAS_API void das_module_bind_structure ( das_module * mod, das_structure * st );
// Register an enumeration (from das_enumeration_make()) into a module.
DAS_API void das_module_bind_enumeration ( das_module * mod, das_enumeration * en );

// --- String allocation ---

// Allocate a copy of 'str' on the daslang string heap.
// Use this to return strings from interop functions; the returned pointer
// is managed by the context and must not be freed by the caller.
DAS_API char * das_allocate_string ( das_context * context, char * str );

// --- Argument getters (aligned) ---
// Extract a C value from a vec4f argument inside an interop function.

DAS_API int    das_argument_int ( vec4f arg );
DAS_API unsigned int   das_argument_uint ( vec4f arg );
DAS_API long long das_argument_int64 ( vec4f arg );
DAS_API unsigned long long das_argument_uint64 ( vec4f arg );
DAS_API int    das_argument_bool ( vec4f arg );
DAS_API float  das_argument_float ( vec4f arg );
DAS_API double das_argument_double ( vec4f arg );
DAS_API char * das_argument_string ( vec4f arg );
DAS_API void * das_argument_ptr ( vec4f arg );
DAS_API das_function * das_argument_function ( vec4f arg );
DAS_API das_lambda * das_argument_lambda ( vec4f arg );
DAS_API das_block * das_argument_block ( vec4f arg );

// --- Argument getters (unaligned) ---
// Extract a C value from a vec4f_unaligned argument pointer.
// Use these in unaligned interop functions.

DAS_API int das_argument_bool_unaligned ( vec4f_unaligned * arg );
DAS_API int das_argument_int_unaligned ( vec4f_unaligned * arg );
DAS_API unsigned int das_argument_uint_unaligned ( vec4f_unaligned * arg );
DAS_API long long das_argument_int64_unaligned ( vec4f_unaligned * arg );
DAS_API unsigned long long das_argument_uint64_unaligned ( vec4f_unaligned * arg );
DAS_API float das_argument_float_unaligned ( vec4f_unaligned * arg );
DAS_API double das_argument_double_unaligned ( vec4f_unaligned * arg );
DAS_API char * das_argument_string_unaligned ( vec4f_unaligned * arg );
DAS_API void * das_argument_ptr_unaligned ( vec4f_unaligned * arg );
DAS_API das_function * das_argument_function_unaligned ( vec4f_unaligned * arg );
DAS_API das_lambda * das_argument_lambda_unaligned ( vec4f_unaligned * arg );
DAS_API das_block * das_argument_block_unaligned ( vec4f_unaligned * arg );

// --- Result setters (aligned) ---
// Pack a C value into vec4f for returning from an aligned interop function.

DAS_API vec4f das_result_void ();
DAS_API vec4f das_result_int ( int r );
DAS_API vec4f das_result_uint ( unsigned int r );
DAS_API vec4f das_result_int64 ( long long r );
DAS_API vec4f das_result_uint64 ( unsigned long long r );
DAS_API vec4f das_result_bool ( int r );
DAS_API vec4f das_result_float ( float r );
DAS_API vec4f das_result_double ( double r );
DAS_API vec4f das_result_string ( char * r );
DAS_API vec4f das_result_ptr ( void * r );
DAS_API vec4f das_result_function ( das_function * r );
DAS_API vec4f das_result_lambda ( das_lambda * r );
DAS_API vec4f das_result_block ( das_block * r );

// --- Result setters (unaligned) ---
// Write a C value into a vec4f_unaligned result pointer.
// Use these in unaligned interop functions.

DAS_API void das_result_void_unaligned ( vec4f_unaligned * result );
DAS_API void das_result_int_unaligned ( vec4f_unaligned * result, int r );
DAS_API void das_result_uint_unaligned ( vec4f_unaligned * result, unsigned int r );
DAS_API void das_result_int64_unaligned ( vec4f_unaligned * result, long long r );
DAS_API void das_result_uint64_unaligned ( vec4f_unaligned * result, unsigned long long r );
DAS_API void das_result_bool_unaligned ( vec4f_unaligned * result, int r );
DAS_API void das_result_float_unaligned ( vec4f_unaligned * result, float r );
DAS_API void das_result_double_unaligned ( vec4f_unaligned * result, double r );
DAS_API void das_result_string_unaligned ( vec4f_unaligned * result, char * r );
DAS_API void das_result_ptr_unaligned ( vec4f_unaligned * result, void * r );
DAS_API void das_result_function_unaligned ( vec4f_unaligned * result, das_function * r );
DAS_API void das_result_lambda_unaligned ( vec4f_unaligned * result, das_lambda * r );
DAS_API void das_result_block_unaligned ( vec4f_unaligned * result, das_block * r );

// --- Compilation policies ---
// CodeOfPolicies controls compiler and runtime behavior: AOT, safety,
// memory limits, and more.  Create a policies object, set individual
// fields, then pass it to das_program_compile_policies().

// Boolean policy flags (on / off).
typedef enum das_bool_policy {
    DAS_POLICY_AOT = 1,                         // Enable ahead-of-time compilation linking
    DAS_POLICY_NO_UNSAFE,                        // Forbid unsafe blocks
    DAS_POLICY_NO_GLOBAL_VARIABLES,              // Forbid module-level var declarations
    DAS_POLICY_NO_GLOBAL_HEAP,                   // Forbid heap allocations for globals
    DAS_POLICY_NO_INIT,                          // Forbid [init] functions
    DAS_POLICY_FAIL_ON_NO_AOT,                   // Treat missing AOT as error
    DAS_POLICY_THREADLOCK_CONTEXT,               // Enable context mutex for threading
    DAS_POLICY_INTERN_STRINGS,                   // Use string interning for the string heap
    DAS_POLICY_PERSISTENT_HEAP,                  // Persistent heap (no GC between calls)
    DAS_POLICY_MULTIPLE_CONTEXTS,                // Enable context-safe code generation
    DAS_POLICY_STRICT_SMART_POINTERS,            // Strict smart pointer rules (var inscope, etc.)
    DAS_POLICY_RTTI,                             // Generate extended RTTI
    DAS_POLICY_NO_OPTIMIZATIONS,                 // Disable all optimizations
    DAS_POLICY_NO_INFER_TIME_FOLDING             // Disable infer-time constant folding
} das_bool_policy;

// Integer policy fields (stack size, heap limits).
typedef enum das_int_policy {
    DAS_POLICY_STACK = 1,                        // Context stack size in bytes (uint32)
    DAS_POLICY_MAX_HEAP_ALLOCATED,               // Max heap allocated in bytes (uint64, 0 = unlimited)
    DAS_POLICY_MAX_STRING_HEAP_ALLOCATED,         // Max string heap allocated in bytes (uint64, 0 = unlimited)
    DAS_POLICY_HEAP_SIZE_HINT,                   // Initial heap size hint in bytes (uint32)
    DAS_POLICY_STRING_HEAP_SIZE_HINT             // Initial string heap size hint in bytes (uint32)
} das_int_policy;

// Create a new policies object with default values.
DAS_API das_policies * das_policies_make();
// Release a policies object.
DAS_API void das_policies_release ( das_policies * policies );
// Set a boolean policy flag. Returns 1 on success, 0 if the flag is unknown.
DAS_API int das_policies_set_bool ( das_policies * policies, das_bool_policy flag, int value );
// Set an integer policy field. Returns 1 on success, 0 if the field is unknown.
DAS_API int das_policies_set_int ( das_policies * policies, das_int_policy field, int64_t value );

// Compile a daslang program with custom compilation policies.
// Same as das_program_compile() but applies the given CodeOfPolicies.
DAS_API das_program * das_program_compile_policies ( char * program_file, das_file_access * access, das_text_writer * tout, das_module_group * libgroup, das_policies * policies );

// --- Context variables ---
// After simulation, global variables declared in daslang are accessible
// by name or by index. findVariable returns -1 if not found.

// Look up a global variable by name. Returns its index, or -1 if not found.
DAS_API int das_context_find_variable ( das_context * context, const char * name );
// Get a raw pointer to the storage of a global variable at index 'idx'.
// Cast to the appropriate C type (e.g. int32_t*, float*, char**).
DAS_API void * das_context_get_variable ( das_context * context, int idx );
// Return the total number of global variables in the context.
DAS_API int das_context_get_total_variables ( das_context * context );
// Return the name of the global variable at index 'idx', or NULL if out of range.
DAS_API const char * das_context_get_variable_name ( das_context * context, int idx );
// Return the size (in bytes) of the global variable at index 'idx', or 0 if out of range.
DAS_API int das_context_get_variable_size ( das_context * context, int idx );

// --- Serialization ---
// Serialize a compiled+simulated program to a binary blob and deserialize
// it back. Deserialized programs skip parsing and type inference.

// Serialize a compiled program to an opaque binary blob.
// The program must have been simulated at least once before serialization.
// Returns a handle to the serialized data; use das_serialized_data_release() to free it.
// 'out_data' receives a pointer to the raw bytes, 'out_size' receives the byte count.
DAS_API das_serialized_data * das_program_serialize ( das_program * program, const void ** out_data, int64_t * out_size );
// Deserialize a program from a raw byte buffer.
// Returns a new program handle that must be released with das_program_release().
DAS_API das_program * das_program_deserialize ( const void * data, int64_t size );
// Release serialized data returned by das_program_serialize().
DAS_API void das_serialized_data_release ( das_serialized_data * blob );

// --- Function info ---

// Return 1 if the function has been AOT-linked, 0 otherwise.
// Use this after simulation to check whether functions execute as native code.
DAS_API int das_function_is_aot ( das_function * func );

// --- Type introspection: base type enum ---

// Base type classification for das_type_info.
// Numeric values match the internal C++ Type enum.
typedef enum das_base_type {
    DAS_TYPE_VOID = 9,
    DAS_TYPE_BOOL,
    DAS_TYPE_INT8,
    DAS_TYPE_UINT8,
    DAS_TYPE_INT16,
    DAS_TYPE_UINT16,
    DAS_TYPE_INT64,
    DAS_TYPE_UINT64,
    DAS_TYPE_INT,
    DAS_TYPE_INT2,
    DAS_TYPE_INT3,
    DAS_TYPE_INT4,
    DAS_TYPE_UINT,
    DAS_TYPE_UINT2,
    DAS_TYPE_UINT3,
    DAS_TYPE_UINT4,
    DAS_TYPE_FLOAT,
    DAS_TYPE_FLOAT2,
    DAS_TYPE_FLOAT3,
    DAS_TYPE_FLOAT4,
    DAS_TYPE_DOUBLE,
    DAS_TYPE_RANGE,
    DAS_TYPE_URANGE,
    DAS_TYPE_RANGE64,
    DAS_TYPE_URANGE64,
    DAS_TYPE_STRING,
    DAS_TYPE_STRUCTURE,
    DAS_TYPE_HANDLE,
    DAS_TYPE_ENUMERATION,
    DAS_TYPE_ENUMERATION8,
    DAS_TYPE_ENUMERATION16,
    DAS_TYPE_ENUMERATION64,
    DAS_TYPE_BITFIELD,
    DAS_TYPE_BITFIELD8,
    DAS_TYPE_BITFIELD16,
    DAS_TYPE_BITFIELD64,
    DAS_TYPE_POINTER,
    DAS_TYPE_FUNCTION,
    DAS_TYPE_LAMBDA,
    DAS_TYPE_ITERATOR,
    DAS_TYPE_ARRAY,
    DAS_TYPE_TABLE,
    DAS_TYPE_BLOCK,
    DAS_TYPE_TUPLE,
    DAS_TYPE_VARIANT
} das_base_type;

// --- Type introspection: flag constants ---

// TypeInfo flags (bitmask).
extern DAS_API uint32_t DAS_TYPEINFO_FLAG_REF;
extern DAS_API uint32_t DAS_TYPEINFO_FLAG_REF_TYPE;
extern DAS_API uint32_t DAS_TYPEINFO_FLAG_CAN_COPY;
extern DAS_API uint32_t DAS_TYPEINFO_FLAG_IS_POD;
extern DAS_API uint32_t DAS_TYPEINFO_FLAG_IS_RAW_POD;
extern DAS_API uint32_t DAS_TYPEINFO_FLAG_IS_CONST;
extern DAS_API uint32_t DAS_TYPEINFO_FLAG_IS_TEMP;
extern DAS_API uint32_t DAS_TYPEINFO_FLAG_IS_SMART_PTR;
extern DAS_API uint32_t DAS_TYPEINFO_FLAG_IS_HANDLED;

// StructInfo flags (bitmask).
extern DAS_API uint32_t DAS_STRUCTINFO_FLAG_CLASS;
extern DAS_API uint32_t DAS_STRUCTINFO_FLAG_LAMBDA;
extern DAS_API uint32_t DAS_STRUCTINFO_FLAG_HEAP_GC;
extern DAS_API uint32_t DAS_STRUCTINFO_FLAG_STRING_HEAP_GC;

// FuncInfo flags (bitmask).
extern DAS_API uint32_t DAS_FUNCINFO_FLAG_INIT;
extern DAS_API uint32_t DAS_FUNCINFO_FLAG_BUILTIN;
extern DAS_API uint32_t DAS_FUNCINFO_FLAG_PRIVATE;
extern DAS_API uint32_t DAS_FUNCINFO_FLAG_SHUTDOWN;

// --- Type introspection: entry points ---

// Return the type info for a global variable at index 'idx'.
// Returns NULL if out of range. The returned pointer is valid for the
// lifetime of the context and must not be freed.
DAS_API das_type_info * das_context_get_variable_type ( das_context * context, int idx );

// Return the total number of simulated functions in the context.
DAS_API int das_context_get_total_functions ( das_context * context );

// Return the function handle at index 'idx', or NULL if out of range.
DAS_API das_function * das_context_get_function ( das_context * context, int idx );

// Return the debug info (FuncInfo) for a function handle.
// Returns NULL if the function has no debug info.
DAS_API das_func_info * das_function_get_info ( das_function * func );

// --- Type introspection: TypeInfo accessors ---

// Return the base type classification (DAS_TYPE_*).
DAS_API das_base_type das_type_info_get_type ( das_type_info * info );

// Return the size in bytes of this type.
DAS_API int das_type_info_get_size ( das_type_info * info );

// Return the alignment in bytes.
DAS_API int das_type_info_get_align ( das_type_info * info );

// Return the flags bitmask (DAS_TYPEINFO_FLAG_*).
DAS_API uint32_t das_type_info_get_flags ( das_type_info * info );

// Return the 64-bit type hash.
DAS_API uint64_t das_type_info_get_hash ( das_type_info * info );

// Return a human-readable type description (e.g. "array<int>", "MyStruct").
// The returned string is in a thread-local buffer and is valid until the
// next call to this function on the same thread.
DAS_API const char * das_type_info_get_description ( das_type_info * info );

// Return the mangled name string for this type.
// Same thread-local lifetime as das_type_info_get_description().
DAS_API const char * das_type_info_get_mangled_name ( das_type_info * info );

// --- Type introspection: composite type navigation ---

// For arrays, pointers, iterators: return the element/pointee type.
// For tables: return the key type.
// Returns NULL if not applicable.
DAS_API das_type_info * das_type_info_get_first_type ( das_type_info * info );

// For tables: return the value type. Returns NULL if not applicable.
DAS_API das_type_info * das_type_info_get_second_type ( das_type_info * info );

// For tuples, variants, function types: return the number of sub-types.
DAS_API int das_type_info_get_arg_count ( das_type_info * info );

// Return the i-th sub-type, or NULL if out of range.
DAS_API das_type_info * das_type_info_get_arg_type ( das_type_info * info, int idx );

// Return the name of the i-th sub-type, or NULL.
DAS_API const char * das_type_info_get_arg_name ( das_type_info * info, int idx );

// For tuples: return the byte offset of the i-th field.
DAS_API int das_type_info_get_tuple_field_offset ( das_type_info * info, int idx );

// For variants: return the byte offset of the i-th field.
DAS_API int das_type_info_get_variant_field_offset ( das_type_info * info, int idx );

// Return the number of fixed-array dimensions (0 = not a fixed-array).
DAS_API int das_type_info_get_dim_count ( das_type_info * info );

// Return the size of the i-th fixed-array dimension.
DAS_API int das_type_info_get_dim ( das_type_info * info, int idx );

// --- Type introspection: structures ---

// For tStructure types: return the StructInfo.
// Returns NULL if the type is not a structure.
DAS_API das_struct_info * das_type_info_get_struct ( das_type_info * info );

// Return the structure name.
DAS_API const char * das_struct_info_get_name ( das_struct_info * info );

// Return the module name that defines this structure.
DAS_API const char * das_struct_info_get_module ( das_struct_info * info );

// Return the number of fields.
DAS_API int das_struct_info_get_field_count ( das_struct_info * info );

// Return the total size of the structure in bytes.
DAS_API int das_struct_info_get_size ( das_struct_info * info );

// Return the structure flags bitmask (DAS_STRUCTINFO_FLAG_*).
DAS_API uint32_t das_struct_info_get_flags ( das_struct_info * info );

// Return the 64-bit hash of the structure.
DAS_API uint64_t das_struct_info_get_hash ( das_struct_info * info );

// Return the name of the i-th field, or NULL if out of range.
DAS_API const char * das_struct_info_get_field_name ( das_struct_info * info, int idx );

// Return the byte offset of the i-th field, or -1 if out of range.
DAS_API int das_struct_info_get_field_offset ( das_struct_info * info, int idx );

// Return the type info for the i-th field, or NULL if out of range.
DAS_API das_type_info * das_struct_info_get_field_type ( das_struct_info * info, int idx );

// --- Type introspection: enumerations ---

// For tEnumeration/tEnumeration8/16/64 types: return the EnumInfo.
// Returns NULL if the type is not an enumeration.
DAS_API das_enum_info * das_type_info_get_enum ( das_type_info * info );

// Return the enumeration name.
DAS_API const char * das_enum_info_get_name ( das_enum_info * info );

// Return the module name that defines this enumeration.
DAS_API const char * das_enum_info_get_module ( das_enum_info * info );

// Return the number of values in the enumeration.
DAS_API int das_enum_info_get_count ( das_enum_info * info );

// Return the name of the i-th value, or NULL if out of range.
DAS_API const char * das_enum_info_get_value_name ( das_enum_info * info, int idx );

// Return the integer value of the i-th entry, or 0 if out of range.
DAS_API int64_t das_enum_info_get_value ( das_enum_info * info, int idx );

// --- Type introspection: function info ---

// Return the function name.
DAS_API const char * das_func_info_get_name ( das_func_info * info );

// Return the C++ mangled name.
DAS_API const char * das_func_info_get_cpp_name ( das_func_info * info );

// Return the number of arguments.
DAS_API int das_func_info_get_arg_count ( das_func_info * info );

// Return the type info for the i-th argument, or NULL if out of range.
DAS_API das_type_info * das_func_info_get_arg_type ( das_func_info * info, int idx );

// Return the name of the i-th argument, or NULL if out of range.
DAS_API const char * das_func_info_get_arg_name ( das_func_info * info, int idx );

// Return the return type info.
DAS_API das_type_info * das_func_info_get_result ( das_func_info * info );

// Return the 64-bit function hash.
DAS_API uint64_t das_func_info_get_hash ( das_func_info * info );

// Return the function flags bitmask (DAS_FUNCINFO_FLAG_*).
DAS_API uint32_t das_func_info_get_flags ( das_func_info * info );

// Return the stack size needed by this function.
DAS_API int das_func_info_get_stack_size ( das_func_info * info );

#ifdef __cplusplus
}
#endif
