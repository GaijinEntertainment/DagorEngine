#pragma once

#include "daScript/misc/platform.h"
#include "daScript/simulate/annotation_arguments.h"

namespace das {

    // bump when CodeOfPolicies changes layout or meaning without changing size — a field
    // reorder/repurpose, or an insert that lands in a padding pocket (sizeof can't see those)
    #define DAS_POLICIES_VERSION    2

    // per-binary linkage for the ABI-stamp functions: at -O0 gcc/clang emit them as weak
    // default-visibility symbols, and the dynamic linker would bind every binary to the FIRST
    // copy — the exe's stamp answered by the exe's own headers, silently defeating the check
    #if defined(__GNUC__) || defined(__clang__)
        #define DAS_ABI_LOCAL __attribute__((visibility("hidden")))
    #else
        #define DAS_ABI_LOCAL
    #endif

    struct CodeOfPolicies {
    // ABI canary - MUST stay the first field. The default initializer compiles into whatever binary
    // CONSTRUCTS the struct (the host exe), while libDaScript checks it against ITS headers at the
    // parse/compile entries (verifyCodeOfPoliciesStamp) - a host built against different daScript
    // headers fails loud there instead of the library reading every policy field at shifted offsets.
        uint32_t    abi_stamp = expected_abi_stamp();
    // Aot config
        bool        aot = false;                        // enable AOT
        /*option*/ bool        aot_lib = false;
        /*option*/ bool        standalone_context = false;         // generate standalone context class in aot mode
        bool        aot_module = false;                 // this is how AOT tool knows module is module, and not an entry point
        bool        aot_macros = false;                 // enables aot of macro code (like 'qmacro_block')
        bool        tune_frozen = false;                // never apply per-box [tune] stamps to this compile (cross-box artifacts: -aot generation, AST serialization)
        bool        paranoid_validation = false;        // todo
        bool        cross_platform = false;             // aot supports platform independent mode
        string      aot_result;                         // Path where to store cpp-result of aot
    // End aot config
        bool        completion = false;                 // this code is being compiled for 'completion' mode
        /*option*/ bool building_documentation = false; // source/reflection docs: disable build-machine-specific transforms
        bool        lint_check = false;                 // this code is being compiled for lint/style checking
        bool        no_lint = false;                    // skip Program::lint() entirely
        bool        no_init_check = false;              // skip the Module::Initialize() assert, most of the time should be false (except maybe dynamic-module discovery)
        bool        export_all = false;                 // when user compiles, export all (public?) functions
        bool        export_public_functions = false;
        bool        serialize_main_module = true;       // if false, then we recompile main module each time
        bool        keep_alive = false;                 // produce keep-alive noodes
        /*option*/ bool        very_safe_context = false;          // context is very safe (does not release old memory from array or table grow, leaves it to GC)
    // error reporting
        int32_t     always_report_candidates_threshold = 6; // always report candidates if there are less than this number
    // infer passes
        /*option*/ int32_t     max_infer_passes = 50;              // maximum number of infer passes
        /*option*/ int32_t     max_call_depth = 50;                // maximum call expression nesting depth during inference
        bool verify_infer_types = false;                       // verify inferred types (to see if there is internal consistency). note - this adds errors to failing tests
    // memory
        /*option*/ uint32_t    stack = 16*1024;                    // 0 for unique stack
        /*option*/ bool        intern_strings = false;             // use string interning lookup for regular string heap
        /*option*/ bool        persistent_heap = false;
        /*option*/ bool        multiple_contexts = false;          // code supports context safety
        /*option*/ uint32_t    heap_size_hint = 65536;
        /*option*/ uint32_t    string_heap_size_hint = 65536;
        /*option*/ bool        solid_context = false;              // all access to varable and function lookup to be context-dependent (via index)
                                                        // this is slightly faster, but prohibits AOT or patches
        bool        macro_context_persistent_heap = true;   // if true, then persistent heap is used for macro context
        bool        macro_context_collect = false;          // GC collect macro context after major passes
    // per-pass AST gc during infer: collect the working root's live tree into a fresh root and
    // swap it in (O(1)), letting the old root's dtor sweep that pass's garbage. Caps the compile
    // memory peak (infer churns many throwaway TypeDecls/Expressions) at ~no time cost.
        /*option*/ bool        gc_infer_collect = true;            // enable per-pass collect+swap during inference
        /*option*/ int32_t     gc_infer_collect_nodes = 25000;     // floor of the collect threshold, in nodes (~2 MB)
        /*option*/ int32_t     gc_infer_collect_pct = 50;          // the threshold is the larger of the floor and this percent of the live set at the last collect
        uint64_t    max_static_variables_size = 0x100000000;   // 4GB
        /*option*/ uint64_t    max_heap_allocated = 0;
        /*option*/ uint64_t    max_string_heap_allocated = 0;
        /*option*/ uint64_t    max_unreserved_size = 64ull<<20;    // a growing array resize past this byte size without a prior reserve panics
        /*option*/ bool        track_allocations = false;          // track where heap allocations came from (line info + comment)
    // rtti
        /*option*/ bool rtti = false;                              // create extended RTTI
    // language
        /*option*/ bool unsafe_table_lookup = false;                // table lookup (tab[key]) to be unsafe
        /*option*/ bool relaxed_pointer_const = false;             // allow const correctness to be relaxed on pointers
        // dagor: gen1 stays the default (upstream defaults it to true) until the tree is converted.
        // Game scripts take the syntax from the loader, but the C++ embedded builtin modules and the
        // .das_project files parse with default policies and are gen1; a gen2 one opts in with
        // `options gen2`.
        bool version_2_syntax = false;                   // use syntax version 2 (per-file `options gen2 = false` still opts out)
        bool gen2_make_syntax = false;                  // only new make syntax is allowed (no [[...]] or [{...}])
        /*option*/ bool relaxed_assign = true;                     // allow = to <- substitution, in certain expressions
        bool no_unsafe = false;
        bool local_ref_is_unsafe = true;                // var a & = ... unsafe. should be
        /*option*/ bool with_module_is_unsafe = false;             // user-written with (module foo) requires unsafe (inliner-generated ones are exempt)
        /*option*/ bool no_global_variables = false;
        /*option*/ bool no_global_variables_at_all = false;
        /*option*/ bool no_global_heap = false;
        /*option*/ bool only_fast_aot = false;
        /*option*/ bool aot_order_side_effects = false;
        /*option*/ bool no_unused_function_arguments = false;
        /*option*/ bool no_unused_block_arguments = false;
        bool allow_block_variable_shadowing = false;
        bool allow_local_variable_shadowing = false;
        bool allow_shared_lambda = false;
        bool ignore_shared_modules = false;
        bool default_module_public = true;              // by default module is 'public', not 'private'
        /*option*/ bool no_deprecated = false;
        /*option*/ bool no_aliasing = false;                       // if true, aliasing will be reported as error, otherwise will turn off optimization
        /*option*/ bool strict_smart_pointers = true;              // collection of tests for smart pointers, like van inscope for any local, etc
        /*option*/ bool no_init = false;                           // if true, then no [init] is allowed in any shape or form
        /*option*/ bool strict_unsafe_delete = true;              // if true, delete of type which contains 'unsafe' delete is unsafe // TODO: enable when need be
        bool no_members_functions_in_struct = false;    // structures can't have member functions
        /*option*/ bool no_local_class_members = true;             // members of the class can't be classes
        /*option*/ bool report_invisible_functions = true;         // report invisible functions (report functions not visible from current module)
        /*option*/ bool report_private_functions = true;           // report private functions (report functions which are not accessible due to private module)
        /*option*/ bool no_unsafe_uninitialized_structures = true; // if true, then unsafe uninitialized structures are not allowed
        /*option*/ bool default_init_containers = true;            // if true, containers construct their elements (resize inits new slots, table index inits fresh slots)
        /*option*/ bool strict_properties = false;                 // if true, then properties are strict, i.e. a.prop = b does not get promoted to a.prop := b
        /*option*/ bool no_writing_to_nameless = true;             // if true, then writing to nameless variables (intermediate on the stack) is not allowed
    // environment
        /*option*/ bool no_optimizations = false;                  // disable optimizations, regardless of settings
        /*option*/ bool no_fold_unsafe = false;                    // keep the `unsafe { }` wrapper in the AST after infer (tooling that walks a compiled program: the scope is invisible once foldUnsafe runs)
        /*option*/ bool fast_math = false;                         // allow VALUE-SAFE float optimizations with bit differences (reassociation, FMA contraction, signed-zero laxity); inf/NaN semantics are preserved — no rcp division, no nan-compare flips (JIT stamps reassoc|nsz|contract only). doubles stay bit-exact unless this is on
        /*option*/ bool disable_dse = false;                       // disable the dead-store-elimination pass
        /*option*/ bool disable_cse = false;                       // disable the common-subexpression-elimination pass
        /*option*/ bool disable_inline = false;                    // disable the [inline] function inliner (calls stay regular calls; declaration-level contract checks - shape, recursion, @@ - still lint)
        /*option*/ bool disable_auto_inline = false;               // disable automatic inlining of block-literal call sites and invoke-of-literal devirtualization ([inline] splicing is unaffected)
        /*option*/ bool auto_inline_functions = true;              // heuristic best-effort inlining of plain calls and operator sites of small same-module [inline]-shaped functions (default ON; silent declines; optimized builds only; disable_auto_inline overrides)
        /*option*/ int32_t auto_inline_cost = 32;                  // auto_inline_functions budget: a callee body up to this many AST nodes is worth splicing (private single-call callees are exempt)
        /*option*/ bool disable_run = false;                       // disable compile-time function evaluation (RunFolding of pure calls over constants)
        /*option*/ bool disable_temp_string_reclaim = false;       // disable the temp-string reclaim pass (fresh-string call results riding the 1-slot dispose queue)
        /*option*/ bool no_infer_time_folding = false;             // disable infer-time constant folding
        bool fail_on_no_aot = true;                     // AOT link failure is error
        bool fail_on_lack_of_aot_export = false;        // remove_unused_symbols = false is missing in the module, which is passed to AOT
        /*option*/ bool log_compile_time = false;                  // if true, then compile time will be printed at the end of the compilation
        /*option*/ bool log_total_compile_time = false;            // if true, then detailed compile time will be printed at the end of the compilation
        /*option*/ bool log_module_compile_time = false;           // if true, every required module logs its own parse / infer (with pass count) / optimize / macro (in infer) / macro mods breakdown + function count; also enables per-context simulate timing and the top-level aggregate summary (CLI: -log-compile-time)
        /*option*/ bool log_optimization = false;                  // log optimizer rewrites (per-pass fired/nothing lines, inline/devirt sites and declines)
        /*option*/ bool log_optimization_passes = false;           // log the AST after every optimizer pass (verbose)
        /*option*/ bool no_fast_call = false;                      // disable fastcall
        /*option*/ bool fusion = true;                             // fuse interpreter nodes into wider superinstructions at simulate time
        /*option*/ bool scoped_stack_allocator = true;             // reuse stack memory after variables out of scope
        /*option*/ bool force_inscope_pod = false;                 // force in-scope for POD-like types
        /*option*/ bool log_inscope_pod = false;                   // log in-scope for POD-like types
        /*option*/ bool force_escape_free = false;                 // escape analysis: statically free non-escaping new-pointer locals at scope exit
        /*option*/ bool force_allocate_on_stack = true;            // escape analysis: stack-allocate non-escaping new-pointer locals (no heap)
        /*option*/ bool force_partial_escape_free = false;         // flow-sensitive escape: build a CFG and free objects on the paths where they don't escape (off = simple EA only, no CFG)
        /*option*/ bool log_escape_analysis = false;               // log escape-analysis static frees
        /*option*/ bool log_gc_time = false;                       // log gc time
    // debugger
        //  when enabled
        //      1. disables [fastcall]
        //      2. invoke of blocks will have extra prologue overhead
        //      3. context always has context mutex
        bool debugger = false;
        /*option*/ bool debug_infer_flag = false;  // set this to true to debug macros for missing "not_inferred"
    // profiler
        // only enabled if profiler is disabled
        // when enabled
        //      1. disables [fastcall]
        bool profiler = false;
    // pinvoke
        /*option*/ bool threadlock_context = false;               // has context mutex
    // jit
        bool jit_enabled = false;                // enable JIT
        // todo: add this params to serialization?
        bool jit_jit_all_functions = true;       // JIT all functions by default
        bool jit_debug_info = false;             // Add debug info to generate binary code
        bool jit_dll_mode = true;                // Create if missing and reuse DLL or JIT compile
        bool jit_exe_mode = false;                // Create executable
        bool jit_emit_object = false;            // Offline AOT: emit a .o (this module only) + a load ctor registering its functions into the AOT library, for static linking into a host
        bool jit_emit_prologue = false;          // Emit prologue for all functions and blocks
        string jit_output_path;                  // Folder to store compiled dll's. By default it'll be _das_root_/.jitted_scripts
        int32_t jit_opt_level = 3u;              // Opt level for LLVM to codegen and IR optimizations
        int32_t jit_size_level = 0u;             // Opt level for LLVM for binary size
        string jit_path_to_shared_lib;           // Path to libDaScript. Optional, we'll try to find it in _das_root_/lib/ if not provided.
        string jit_path_to_linker;               // Path to linker. Optional, we'll use lld-link from LLVM on Windows (MSVC), clang on mingw, c++ otherwise.
    // dll loading
        vector<string> dll_search_paths;          // additional search paths for dll loading
    // one-liners
        /*option*/ bool temp_table_lint_warning = false;
        bool module_cache = false;               // a script's compile_file runs under the default module cache
        /*option*/ uint32_t max_fast_call_depth = 0;  // interpreter only; 0 = unchecked. last: a cached JIT DLL binds earlier fields by offset

        // the abi_stamp truth (a member so the NSDMI above can call it - complete-class context).
        // Low byte 0 keeps a pre-stamp libDaScript reading this word seeing aot == false -
        // cosmetic only; the stale-LIBRARY direction is unrecoverable either way (every later
        // field shifts). The checked direction is the stale-HOST one.
        static constexpr DAS_ABI_LOCAL uint32_t expected_abi_stamp() {
            return (uint32_t(DAS_POLICIES_VERSION) << 24) | (uint32_t(sizeof(CodeOfPolicies)) << 8);
        }
    };

    // the pure predicate (tested in tests-cpp), and the fatal wrapper called at the
    // policy-receiving entries (parseDaScript/compileDaScript): prints and exits on a stamp
    // mismatch - no policy field can be trusted when the host wrote a different layout
    DAS_CC_API bool checkCodeOfPoliciesStamp ( const CodeOfPolicies & policies );
    DAS_CC_API void verifyCodeOfPoliciesStamp ( const CodeOfPolicies & policies );
}
