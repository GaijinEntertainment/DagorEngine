options gen2
options indenting = 4
options strict_smart_pointers = false
options stack = 4_194_304

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_dll_utils
require llvm/daslib/llvm_exe
require llvm/daslib/llvm_aot
require llvm/daslib/llvm_jit
require llvm/daslib/llvm_jit_common
require llvm/daslib/llvm_jit_intrin
require llvm/daslib/llvm_jit_cli

require daslib/ast_boost
require daslib/strings_boost
require daslib/defer

require daslib/fio
require daslib/option
require daslib/result
require jit

module llvm_jit_run shared private

//! JIT codegen driver shared by the `jit_llvm` simulate-macro (llvm_macro) and
//! the `utils/jit` tool. `run_jit` performs the whole compile->IR->optimize->
//! emit(.dll/.exe/.wasm) sequence for an already-simulated program+context, so
//! callers do not need to inject the `just_in_time` module at runtime.

var LINK_WHOLE_LIB = false // when true, standalone exe links against the whole library; when false, only against runtime

// nolint:STYLE014
// Bump this constant whenever LLVM codegen / linking logic changes in a way that
// invalidates cached DLLs (e.g. edits to llvm_jit.das, llvm_macro.das, llvm_jit_common.das,
// runtime helper ABI, default target triple). Cache filenames fold this in, so a bump
// makes every previously written DLL miss the cache on the next run and get GC'd.
let LLVM_JIT_CODEGEN_VERSION : uint64 = 0x4bul   // block annotationData resolved via jit_ad_by_sid instead of a baked glob (0x4a: shuffle: native aarch64 tbl1 arm + lattice >> lowering)

let JIT_FNV_PRIME : uint64 = 1099511628211ul

// `options fast_math` stamps every FP op with VALUE-SAFE flags (reassoc|nsz|contract) before opt —
// NON-bit-exact, default off, +8-10% e2e. nnan|ninf|arcp|afn are deliberately NOT stamped: engine
// kernels make real infs and arcp+ninf's rcp-estimate div computes inf*0=NaN (tests/jit/fast_math_specials.das).
var g_jit_fast_math = false

def private apply_fast_math_to_module(m : LLVMOpaqueModule?) {
    var fn = LLVMGetFirstFunction(m)
    while (fn != null) {
        var bb = LLVMGetFirstBasicBlock(fn)
        while (bb != null) {
            var inst = LLVMGetFirstInstruction(bb)
            while (inst != null) {
                // authored flags win — the blanket stamp only fills instructions with no explicit FMF
                if (LLVMCanValueUseFastMathFlags(inst) != 0 && LLVMGetFastMathFlags(inst) == 0u) {
                    LLVMSetFastMathFlags(inst, 0x29u)   // reassoc|nsz|contract (see header note)
                }
                inst = LLVMGetNextInstruction(inst)
            }
            bb = LLVMGetNextBasicBlock(bb)
        }
        fn = LLVMGetNextFunction(fn)
    }
}

// nolint:STYLE014
// Loop tune hints (vectorize / vectorize_width / unroll / unroll_count / interleave_count)
// live on ExprFor / ExprWhile annotations and reach ONLY the JIT — attach_loop_metadata
// turns them into LLVM !llvm.loop metadata. They never enter the interpreter SimNode tree
// that get_function_aot_hash walks, so the semantic hash is blind to them: a re-tuned
// kernel ([tuned] restamping a different perm from a changed box_profile.json) has an
// IDENTICAL aot hash and would silently reuse a stale DLL. Fold them into the JIT-only
// cache key here, alongside LLVM_JIT_CODEGEN_VERSION / g_jit_fast_math.
// Fold the ONE value field the arg's type says is live. bValue/iValue/fValue share a C++
// union — reading a non-live member is UB (a bool load of vectorize_width=8 aborts ubsan
// and lets clang -O2 miscompile: chunk workers died with SIGSEGV on linux Release CI).
// Type coverage stays total: any basicType re-keys the DLL via its own live field.
def private fold_annotation_args(args : AnnotationArgumentList; seed : uint64) : uint64 {
    var h = seed
    for (a in args) {
        h = (h ^ hash(a.name)) * JIT_FNV_PRIME
        h = (h ^ uint64(int(a.basicType))) * JIT_FNV_PRIME
        if (a.basicType == Type.tBool) {
            h = (h ^ (a.bValue ? 1ul : 0ul)) * JIT_FNV_PRIME
        } elif (a.basicType == Type.tInt) {
            h = (h ^ uint64(a.iValue)) * JIT_FNV_PRIME
        } elif (a.basicType == Type.tFloat) {
            h = (h ^ hash(a.fValue)) * JIT_FNV_PRIME
        } elif (a.basicType == Type.tString) {
            h = (h ^ hash(a.sValue)) * JIT_FNV_PRIME
        }
    }
    return h
}

class private LoopHintHashVisitor : AstVisitor {
    h : uint64
    def override preVisitExprFor(expr : ExprFor?) : void {
        h = fold_annotation_args(expr.annotations, h)
    }
    def override preVisitExprWhile(expr : ExprWhile?) : void {
        h = fold_annotation_args(expr.annotations, h)
    }
}

// Complete under jit_all_functions (the default): every used function is a top-level entry
// in `funcs`, so folding each function's own body covers the whole compiled program.
def private fold_loop_hints(funcs : array<FunctionPtr>; seed : uint64) : uint64 {
    var vis = new LoopHintHashVisitor()
    vis.h = seed
    make_visitor(*vis) $(adapter) {
        for (fn in funcs) {
            visit(fn.body, adapter)
        }
    }
    let r = vis.h
    unsafe { delete vis }
    return r
}

// [llvm_code] annotation args are generator params: they change emitted IR without touching the
// reference body, so the aot hash is blind — fold them like loop hints. A generator CODE change is
// invisible too: bump LLVM_JIT_CODEGEN_VERSION when a generator's emission changes for fixed args.
def private fold_llvm_code_hints(funcs : array<FunctionPtr>; seed : uint64) : uint64 {
    var h = seed
    for (fn in funcs) {
        for (ann in fn.annotations) {
            if (ann.annotation.name == "llvm_code") {
                h = (h ^ hash("llvm_code")) * JIT_FNV_PRIME
                h = fold_annotation_args(ann.arguments, h)
            }
        }
    }
    return h
}

// True when the program jits any [llvm_code] kernel — those emit host-specific IR (e.g. AVX2) a
// generic x86-64 exe can't legalize, so a -exe carrying them must target the current box. Scanned
// BEFORE target flags: the generators' decline predicates consult g_target_*, so order matters.
def private has_generated_kernel(prog : Program?; jit_all_functions : bool) : bool {
    var found = false
    prog |> for_each_module() $(mod) {
        mod |> for_each_function("") $(fun) {
            if (fun.flags.used && (jit_all_functions || fun.moreFlags.requestJit) && !fun.moreFlags.requestNoJit) {
                for (ann in fun.annotations) {
                    if (ann.annotation.name == "llvm_code") {
                        found = true
                    }
                }
            }
        }
    }
    return found
}

def jit_dll_basename(funcs : array<FunctionPtr>; opt_level : int; size_level : int;
                     emit_prologue : bool; debug_info : bool; target_triple : string) : string {
    var h = LLVM_JIT_CODEGEN_VERSION
    for (fn in funcs) {
        h = (h ^ get_function_aot_hash(fn)) * JIT_FNV_PRIME
    }
    h = fold_loop_hints(funcs, h)   // JIT-only loop tune hints (see LoopHintHashVisitor)
    h = fold_llvm_code_hints(funcs, h)   // [llvm_code] generator params (annotation args)
    h = (h ^ uint64(opt_level)) * JIT_FNV_PRIME
    h = (h ^ uint64(size_level)) * JIT_FNV_PRIME
    h = (h ^ (emit_prologue ? 1ul : 0ul)) * JIT_FNV_PRIME
    h = (h ^ (debug_info ? 1ul : 0ul)) * JIT_FNV_PRIME
    h = (h ^ (g_jit_fast_math ? 1ul : 0ul)) * JIT_FNV_PRIME   // fast-math changes codegen → distinct cache
    // EH vs longjmp hosts emit different unwind tables — must not share a cache entry (panic would crash)
    h = (h ^ (das_is_exceptions_enabled() ? 1ul : 0ul)) * JIT_FNV_PRIME
    // AVX2 gates the x64 intrinsic table — an AVX2-jitted DLL must not cache-hit an AVX2-less host (illegal instr)
    h = (h ^ (cpu_supports("avx2") ? 1ul : 0ul)) * JIT_FNV_PRIME
    // Matrix-tier cpuid bits (vnni256/avx512bw/avx512vnni) + DAS_JIT_X64_FORCE_FEATURES key emission the same way
    var isa = 0ul
    if (cpu_supports("avxvnni")) {
        isa |= 1ul
    }
    if (cpu_supports("avx512f")) {
        isa |= 2ul
    }
    if (cpu_supports("avx512bw")) {
        isa |= 4ul
    }
    if (cpu_supports("avx512vl")) {
        isa |= 8ul
    }
    if (cpu_supports("avx512vnni")) {
        isa |= 16ul
    }
    if (cpu_supports("avxvnniint8")) {
        isa |= 32ul
    }
    h = (h ^ isa) * JIT_FNV_PRIME
    let forced = get_env_variable("DAS_JIT_X64_FORCE_FEATURES")
    if (!empty(forced)) {
        h = (h ^ hash(forced)) * JIT_FNV_PRIME
    }
    // aarch64 twin (DAS_JIT_ARM64_FORCE_FEATURES, e.g. "i8mm"): a forced artifact must not cache-hit a normal run
    let forced_arm = get_env_variable("DAS_JIT_ARM64_FORCE_FEATURES")
    if (!empty(forced_arm)) {
        h = (h ^ hash(forced_arm)) * JIT_FNV_PRIME
    }
    // Cross-compile artifacts key on their triple — a --jit-target DLL must not cache-hit a host run (foreign-arch dlopen)
    if (!empty(target_triple)) {
        h = (h ^ hash(target_triple)) * JIT_FNV_PRIME
    }
    return "{h}"
}

def jit_gc_stale_dlls(subdir : string; keep_basename : string) {
    // Delete files in subdir whose basename != current hash (stale .dll + linker artifacts: .o/.lib/.exp/.pdb)
    let keep_prefix = "{keep_basename}."
    dir(subdir) $(fname) {
        if (fname == "." || fname == ".." || fname |> starts_with(keep_prefix)) return
        let full_path = "{subdir}/{fname}"
        var err : string
        let ok = remove(full_path, err)
        if (ok && empty(err)) {
            to_log(LOG_INFO, "LLVM JIT: GC stale artifact {full_path}\n")
        } else {
            to_log(LOG_WARNING, "LLVM JIT: failed to GC stale artifact {full_path}: {err}\n")
        }
    }
}

struct JitState {
    dll : void?
    ee : void?
    ctx : void?
}

[macro_function]
def finalize_jit(use_dll : bool; gen_exe : bool; lib_handle : DLLHandle?) : JitState? {
    verify(!(is_in_completion() || is_compiling_macros()))
    verify(!(g_mod == null))    // prevent double finalize
    if (LLVM_LOG_RESULT) {
        LLVMDumpModule(g_mod)
        fflush(fstderr())
    }
    var result : JitState? = null;
    if (use_dll || gen_exe) {
        // Note: ExecutionEngine is owner of the g_mod
        LLVMDisposeExecutionEngine(g_engine);
        LLVMContextDispose(g_ctx);

        // JitState is dll only.
        if (use_dll) {
            result = new JitState(dll = lib_handle.handle, ee = null, ctx = null)
        } else {
            // Nothing to finalize.
            result = new JitState(dll = null, ee = null, ctx = null)

        }
    } else {
        g_engine |> LLVMRunStaticConstructors()

        // JitState is engine + ctx.
        result = new JitState(dll = null, ee = g_engine, ctx = g_ctx)
    }
    g_engine = null
    g_ctx = null
    g_mod = null
    unsafe {
        delete g_str2v
    }
    return result
}

// After a codegen failure the global LLVM state is left built but unusable, and init_jit bails while
// g_mod != null — so dispose + null it (mirrors finalize_jit) to let a one-process bulk run survive an
// unjittable file instead of the next run_jit inheriting it and cascade-failing.
[macro_function]
def reset_jit_globals_after_failure() {
    if (g_engine != null) {
        LLVMDisposeExecutionEngine(g_engine)    // engine owns g_mod, frees it too
    } elif (g_ctx != null) {
        LLVMContextDispose(g_ctx)
    }
    g_engine = null
    g_ctx = null
    g_mod = null
    g_failed = false
    unsafe {
        delete g_str2v
    }
    // Drain codegen accumulators the skipped generate_global_ctors_dtors would have cleared (stale fileinfo crashes next file)
    reset_codegen_accumulators()
}

//! Run the full JIT backend for an already-compiled & simulated program: collect the functions to
//! JIT (honoring [jit]/[no_jit] and jit_all), content-address the DLL, generate and optimize IR,
//! and emit a .dll / .exe / .wasm per the program's jit policies. Returns true.
[macro_function]
def public run_jit(prog : Program?; var ctx : Context?) : bool {  // nolint:STYLE037,STYLE038 - the JIT backend driver: flat policy/flag resolution feeding one linear codegen pipeline; the phases share the module/target state and do not survive a split
    // AOT-object host binding: compiled jit-aware + policies.aot, so linkCppAot binds each
    // function from the statically-linked .o (SimNode_Jit) -- no in-memory codegen needed
    // here. (Emit mode sets jit_emit_object, never aot, so it skips this.)
    if (prog.policies.aot && !prog.policies.jit_emit_object) {
        return true
    }
    var funcs : array<FunctionPtr>
    var disabled : array<FunctionPtr>
    var disableJitVisitor = new DisableJitVisitor()

    let r <- parse_args(type<JitCliOptions>)
    let cli_opts = r |> unwrap_or_default
    assume jit_all_functions = prog.policies.jit_jit_all_functions
    let debug_info = cli_opts.debug_info |> unwrap_or(prog.policies.jit_debug_info)
    let dump_ir = cli_opts.dump_ir |> unwrap_or(false)
    if (prog.policies.jit_dll_mode && !das_is_dll_build()) {
        to_log(LOG_WARNING, "LLVM JIT: dll mode is enabled, however " +
            "daslang build is static, disabling dll mode.\n")
    }
    let gen_exe = prog.policies.jit_exe_mode
    // Drives the $::is_standalone_exe JIT intrinsic in llvm_jit_intrin so daslang
    // code can constant-fold to true when this codegen run produces a standalone exe.
    llvm_jit_intrin::g_jit_exe_mode = gen_exe
    defer() { llvm_jit_intrin::g_jit_exe_mode = false; }
    let exe_main = "main" // prog.policies.jit_entry_point |> empty() ? "main" : prog.policies.jit_entry_point
    // --jit-compile-only: build + verify + optimize (+dump), write/load/install nothing —
    // the program runs interpreted. Also bypasses the DLL cache probe below, so the
    // generators always run (this is the cross-emission smoke rail).
    let compile_only = cli_opts.compile_only |> unwrap_or(false)
    // Offline AOT: emit the whole used function set to a .o for static linking, with a
    // load ctor registering them into the AOT library. Mutually exclusive with dll
    // mode (no in-memory engine work, no instrument).
    let emit_aot_object = prog.policies.jit_emit_object
    // Emit builtin-accessor globals internal + null in object mode (re-resolved by
    // the emit_aot_object_globinit ctor); reset after this run.
    g_emit_aot_object_globals = emit_aot_object
    if (emit_aot_object) {
        g_aot_func_globs |> clear()
    }
    defer() { g_emit_aot_object_globals = false; }
    let use_dll = prog.policies.jit_dll_mode && das_is_dll_build() && !gen_exe && !compile_only && !emit_aot_object
    let opt_level = cli_opts.opt_level |> unwrap_or(prog.policies.jit_opt_level)
    let size_level = cli_opts.size_level |> unwrap_or(prog.policies.jit_size_level)
    // This policy must be set before compilation so stack-frame sizes include the prologue.
    let emit_prologue = prog.policies.emit_prologue
    // If you're sure `dynamic` functions is not reachable set this flag
    // to `true` and fail in runtime on `jit_trap` if they turns
    // out to be reachable.
    let exe_strict = true
    assume config_out_path = string(prog.policies.jit_output_path)
    var output_path = config_out_path |> empty() ? ".jitted_scripts/{prog.thisNamespace}" : config_out_path
    assume path_to_shared_lib = string(prog.policies.jit_path_to_shared_lib)
    // CLI --jit-path-to-linker wins over the script-level policy.
    let path_to_linker = cli_opts.path_to_linker |> unwrap_or(string(prog.policies.jit_path_to_linker))
    // Cross-compile triple: --jit-target=<triple> CLI wins over the
    // script-level `options jit_target = "..."`.
    let opt_target = (prog._options |> find_arg("jit_target")) ?as tString ?? ""
    let target_triple = cli_opts.target |> unwrap_or(opt_target)
    let gen_wasm = gen_exe && (target_triple |> starts_with("wasm"))
    // Optional CLI override for the wasm runtime archive location. Falls
    // back to `options jit_runtime_lib = "..."` then write_wasm's
    // find_runtime_lib() auto-locate under web/output/lib/.
    let runtime_lib_override = cli_opts.runtime_lib |> unwrap_or((prog._options |> find_arg("jit_runtime_lib")) ?as tString ?? "")
    // --jit-emit-object: emit the wasm object and skip the emcc link (the caller
    // links it, e.g. daspkg release wasm). Sidesteps the link entirely, including
    // the Windows popen-quoting failure path in link_wasm.
    let emit_object = cli_opts.emit_object |> unwrap_or(false)
    // --jit-threads: emit a -pthread-compatible wasm object (+atomics,+bulk-memory).
    // Set the codegen global before any IR is built so both the data-layout machine
    // and write_wasm's emission machine pick up the threaded feature set.
    g_target_wasm_threads = cli_opts.threads |> unwrap_or(false)
    // --jit-check-abi: emit the handled-type host-vs-target layout safe-check into
    // initialize_modules(). Set before codegen so llvm_exe's sweep sees the flag.
    g_jit_check_handled_abi = cli_opts.check_abi |> unwrap_or(false)
    // Extra args appended verbatim to the host linker cmd. CLI > script option.
    let linker_string = cli_opts.linker_string |> unwrap_or((prog._options |> find_arg("jit_linker_string")) ?as tString ?? "")

    // Set global options
    LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS = prog._options |> find_arg("jit_vec3_ldu") ?as tBool ?? LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS_DEFAULT
    // `options fast_math` (or policies.fast_math) is the one knob; `_jit_fast_math` kept as a legacy alias
    g_jit_fast_math = (prog._options |> find_arg("fast_math") ?as tBool ?? prog.policies.fast_math) || (prog._options |> find_arg("_jit_fast_math") ?as tBool ?? false)

    // nolint:STYLE014
    // the disable pass consults target-gated intrinsic tables (has_intrinsic) — the
    // g_target_* truth must be set BEFORE it runs, not first at init_jit time below.
    // Standalone exes target a GENERIC CPU (write_exe use_host_cpu=false default), so
    // host cpuid features must not leak into their emission (host_features=false) —
    // EXCEPT a native exe carrying [llvm_code] kernels: a local-use artifact by
    // definition, it targets the current box in BOTH the flags and the machine (the ONE
    // truth, decided here — flags and machine must never diverge: host-F16C flags on a
    // generic machine lower fp16 through libcalls msvcrt never links, and generic flags
    // on a host machine decline every generated kernel to its reference body).
    let exe_host_cpu = gen_exe && empty(target_triple) && has_generated_kernel(prog, jit_all_functions)
    if (exe_host_cpu) {
        to_log(LOG_INFO, "LLVM EXE: targeting the current box (CPU + features) - the build carries [llvm_code] kernels\n")
    }
    // AOT objects target the host CPU: the .o is statically linked into a binary that runs on this
    // box (same contract as the JIT), so bake host features — this force-appends +dotprod on aarch64,
    // without which the dasLLAMA Q8 SDOT kernels have no instruction to select and codegen aborts.
    let use_host_cpu = emit_aot_object ? true : (!gen_exe || exe_host_cpu)
    init_jit_target_flags(target_triple, use_host_cpu)
    make_visitor(*disableJitVisitor) $(disableJitVisitorAdapter) {
        prog |> for_each_module() $(mod) {
            mod |> for_each_function("") $(fun) {
                let rqj = fun.moreFlags.requestJit
                fun.moreFlags.requestJit = false
                // In object mode emit the whole used, non-noAot set (all modules +
                // per-program generic instantiations), mirroring C++ AOT whole-program
                // coverage; noAot functions interpret as Program::linkCppAot skips them.
                if (fun.flags.used && (jit_all_functions || rqj) && (!emit_aot_object || !fun.flags.noAot)) {
                    if (!fun.moreFlags.requestNoJit) {
                        disableJitVisitor.disable = false
                        // if DisableJitVisitor ever grows a preVisitFunction check again, it must
                        // be dispatched explicitly here — `visit(fun)` never calls preVisitFunction
                        fun |> visit(disableJitVisitorAdapter)
                        if (!disableJitVisitor.disable) {
                            fun.moreFlags.requestJit = true
                            funcs |> emplace(fun)
                        } else {
                            to_log(LOG_INFO, "LLVM JIT: disabled {fun.name}\n")
                        }
                    } else {
                        disabled |> emplace(fun)
                    }
                }
            }
        }
    }
    unsafe {
        delete disableJitVisitor
    }
    // emit_aot_object always writes an object even with no functions to emit (a fully-no_aot
    // module): an empty .o with just the fileinfo ctor, so the build always has the file it
    // expects and those functions interpret at load (as Program::linkCppAot skips them).
    if (!empty(funcs) || emit_aot_object) {
        let totalTime = ref_time_ticks()
        // Content-address the DLL: distinct (AST + codegen version + opt flags) → distinct files, same
        // inputs → same filename → cache hit. Only in dll-mode with the default output path; a pinned
        // jit_output_path or an exe keeps the user's path.
        var dll_hash_basename : string
        if (use_dll && config_out_path |> empty()) {
            dll_hash_basename = jit_dll_basename(funcs, opt_level, size_level, emit_prologue, debug_info, target_triple)
            output_path = ".jitted_scripts/{prog.thisNamespace}/{dll_hash_basename}"
        }
        // Pass target_triple so init_jit can pin the module data layout
            // BEFORE IR construction (wasm32 cross-compile needs 4-byte pointer
            // sizes baked into GEP folding from the very first IR instruction).
            init_jit(opt_level |> uint, target_triple, use_host_cpu)

        // "Debug Info Version" gates all !dbg metadata (absent -> silently stripped);
        // "CodeView" flips COFF debug emission from DWARF to CodeView so lld-link
        // /DEBUG can build PDB line tables (windows targets only).
        if (debug_info) {
            let div_key = "Debug Info Version"
            LLVMAddModuleFlag(g_mod, LLVMModuleFlagBehavior.Warning, div_key, uint64(long_length(div_key)),
                LLVMValueAsMetadata(LLVMConstInt(LLVMInt32TypeInContext(g_ctx), 3ul, 0)))
            // host_jit_triple() is "" on MSVC hosts (it only disambiguates mingw arches) —
            // LLVM's default triple is the actual host answer.
            let triple = (target_triple |> empty()) ? LLVMGetDefaultTargetTriple() : target_triple
            if (triple |> find("windows") != -1) {
                let cv_key = "CodeView"
                LLVMAddModuleFlag(g_mod, LLVMModuleFlagBehavior.Warning, cv_key, uint64(long_length(cv_key)),
                    LLVMValueAsMetadata(LLVMConstInt(LLVMInt32TypeInContext(g_ctx), 1ul, 0)))
            }
        }

        var jit_flags : LlvmJitFlags
        jit_flags.emit_prologue = emit_prologue
        jit_flags.need_di = debug_info
        let jit_mode = emit_aot_object ? LlvmJitMode.LLVM_AOT : (gen_exe ? LlvmJitMode.EXE : LlvmJitMode.JIT)

        let attrs = new Attributes(g_ctx)
        var uids = create_uid_nodes(prog, funcs, gen_exe)
        var fake_visitor = new LlvmJitVisitor(ctx, g_prim_t, uids, jit_flags, jit_mode, null, attrs)
        ast_gc_guard() {
            for (fun in funcs) {
                fake_visitor->add_llvm_functions(fun)
            }
        }
        unsafe {
            delete fake_visitor
        }
        var g_dynamic_lib_handle : DLLHandle? = null;
        var recompile_prog = true
        if (use_dll) {
            g_dynamic_lib_handle = get_dll_by_path(output_path)
            let (missing, disabling) = get_dll_missing(funcs, disabled, g_dynamic_lib_handle, uids)
            let dll_changed = !(missing |> empty() && disabling |> empty())
            recompile_prog = dll_changed
            if (recompile_prog) {
                g_dynamic_lib_handle.reset();
                to_log(LOG_INFO, "LLVM JIT: DLL cache miss, codegen for {output_path}.dll\n")
            } else {
                to_log(LOG_INFO, "LLVM JIT: DLL cache hit {output_path}.dll\n")
            }
        }
        var jit_has_externals = false
        if (recompile_prog) {
            for (fun in funcs) {
                jit_has_externals = generate_llvm(ctx, g_prim_t, uids, attrs, fun, jit_flags, jit_mode) || jit_has_externals
                let fnmna = uids.get_dll_fn_name(fun)
                var fn_impl = LLVMGetNamedFunction(g_mod, fnmna.impl())
                var fn = LLVMGetNamedFunction(g_mod, fnmna.publ())
                assume maybe_mod = empty(prog.thisModuleName) ? "" : "in module {prog.thisModuleName}"
                if (!LLVMVerifyFunction(fn_impl, LLVMVerifierFailureAction.LLVMPrintMessageAction, false)) {
                    // to_log(LOG_ERROR, "LLVM JIT: {LLVMPrintModuleToString(g_mod)}\n");
                    reset_jit_globals_after_failure()
                    panic("Internal jit error. Failed to get IR of '{fun.name} implementation' {maybe_mod}.\n")
                }
                if (!LLVMVerifyFunction(fn, LLVMVerifierFailureAction.LLVMPrintMessageAction, false)) {
                    // to_log(LOG_ERROR, "LLVM JIT: {LLVMPrintModuleToString(g_mod)}\n");
                    reset_jit_globals_after_failure()
                    panic("Internal jit error. Failed to get IR of '{fun.name}' in module {maybe_mod}.\n")
                }
            }
            if (gen_exe) {
                let res = inject_main(ctx, g_ctx, prog, exe_main, g_mod, g_prim_t, uids, exe_strict,
                    cli_opts.register_all_modules |> unwrap_or(false))
                LINK_WHOLE_LIB = res.link_whole_lib
                if (res.fn == null) {
                    finalize_jit(use_dll, gen_exe, g_dynamic_lib_handle)
                    // JIT failed. Couldn't create executable.
                    // Still return true, codegen ran correctly.
                    return true
                }
            }
            if (!LLVMVerifyModule(g_mod, LLVMVerifierFailureAction.LLVMPrintMessageAction, false)) {
                g_failed = true
            }
            if (g_failed) {
                let fn_names = join([ for (fn in funcs); "{fn.name}" ], ",")
                reset_jit_globals_after_failure()
                panic("Internal jit error. Failed to get IR for functions {fn_names}.\n")
            }
            // Offline AOT object: build the self-registration + glob-re-resolution ctors (llvm_aot pass).
            var aot_ctor : LLVMOpaqueValue? = null
            var aot_dtor : LLVMOpaqueValue? = null
            if (emit_aot_object) {
                var pair = build_llvm_aot_ctors(g_ctx, g_mod, g_prim_t, funcs, uids, prog)
                aot_ctor = pair._0
                aot_dtor = pair._1
            }
            generate_global_ctors_dtors(g_prim_t, emit_aot_object ? false : !(use_dll || gen_exe), aot_ctor, aot_dtor)
            if (g_jit_fast_math) {
                apply_fast_math_to_module(g_mod)   // EXPERIMENT: stamp all FP ops fast (non-bit-exact ceiling)
            }
            optimize_llvm_module(opt_level |> uint(), size_level |> uint(), LLVM_INLINE_THRESHOLD, use_host_cpu)
            if (dump_ir) {
                // Let's remove unused attributes before we print IR.
                var pmo = LLVMCreatePassBuilderOptions()
                let dce_err = LLVMRunPasses(g_mod, "globaldce", null, pmo)
                if (dce_err != null) {
                    let dce_msg = LLVMGetErrorMessage(dce_err)
                    to_log(LOG_WARNING, "LLVM globaldce failed: {dce_msg}\n")
                    LLVMDisposeErrorMessage(dce_msg)
                }
                LLVMDisposePassBuilderOptions(pmo)
                let module_str = LLVMPrintModuleToString(g_mod)
                let msg = clone_string(module_str)
                LLVMDisposeMessage(module_str)
                to_log(LOG_INFO, "LLVM JIT: {msg}\n");
            }
            if (!LLVMVerifyModule(g_mod, LLVMVerifierFailureAction.LLVMPrintMessageAction, false)) {
                g_failed = true
            }
            if (g_failed) {
                let fn_names = join([ for (fn in funcs); "{fn.name}" ], ",")
                reset_jit_globals_after_failure()
                panic("Internal jit error. Failed to get IR for functions {fn_names}.\n")
            }
            if (compile_only) {
                to_log(LOG_INFO, "LLVM JIT: compile-only - module built, optimized and verified; no artifact written, nothing installed\n")
            } elif (emit_aot_object) {
                emit_object_only(g_mod, output_path, use_host_cpu)
            } elif (gen_wasm) {
                mkdir_rec(dir_name(output_path))
                // path_to_linker is reused as the optional emcc override
                // for the wasm link (mirrors how it's used for the host
                // linker, lld-link/clang/c++, on the host path).
                write_wasm(g_mod, output_path, target_triple, path_to_linker, jit_has_externals, runtime_lib_override, emit_object)
            } elif (gen_exe) {
                mkdir_rec(dir_name(output_path))
                write_exe(g_mod, output_path, path_to_shared_lib, path_to_linker, linker_string, LINK_WHOLE_LIB, exe_host_cpu, debug_info)
            } elif (use_dll) {
                mkdir_rec(dir_name(output_path))
                write_dll(g_mod, output_path, path_to_shared_lib, path_to_linker, linker_string, LINK_WHOLE_LIB, debug_info)

                // Open just stored DLL, so we can keep working.
                g_dynamic_lib_handle.reset()
                g_dynamic_lib_handle = get_dll_by_path(output_path)
                let (missing, disabling) = get_dll_missing(funcs, disabled, g_dynamic_lib_handle, uids)
                assert(missing |> empty() && disabling |> empty())

                // Clean up stale DLLs in the per-script subdirectory.
                if (!(dll_hash_basename |> empty())) {
                    jit_gc_stale_dlls(dir_name(output_path), dll_hash_basename)
                }
            }
        }
        // Object emission is done; no live engine needed. finalize_jit teardown is skipped on
        // this early return, so dispose the engine init_jit created (owns g_mod) + the context
        // here, else LeakSanitizer flags MCJIT::createJIT.
        if (emit_aot_object) {
            LLVMDisposeExecutionEngine(g_engine)
            LLVMContextDispose(g_ctx)
            g_engine = null
            g_ctx = null
            g_mod = null
            unsafe {
                delete g_str2v
            }
            return true
        }
        if (use_dll) {
            assert(g_dynamic_lib_handle.handle != null)
            var extern_resolver = new ResolveExternVisitor(g_dynamic_lib_handle, g_prim_t, ctx, clone(uids))
            make_visitor(*extern_resolver) $(adapter_resolve) {
                for (fn in funcs) {
                    visit(fn, adapter_resolve)
                }
            }
        }
        if (!gen_exe && !compile_only) {
            for (fun in funcs) {
                let mangled_name = get_mangled_name(fun)
                let MNH = hash(mangled_name)
                let simfun = get_function_by_mangled_name_hash(MNH, *ctx)
                var gen = generate_llvm_code(g_dynamic_lib_handle, uids.get_dll_fn_name(fun), use_dll)
                if (gen != null) {
                    gen |> unsafe(instrument_jit(simfun, fun.at, *ctx))
                } else {
                    unsafe(remove_jit(simfun))
                }
            }
        }
        // compile-only takes the exe-style teardown: dispose the engine/context outright,
        // never RunStaticConstructors on (or install from) a module that may target a
        // foreign arch.
        var result = finalize_jit(use_dll, gen_exe || compile_only, g_dynamic_lib_handle)

        set_jit_state(*ctx, result.dll, result.ee, result.ctx)

        // always-on: a silent multi-second jit is indistinguishable from a hang (or from tune)
        let totalDt = double(get_time_usec(totalTime)) / 1000000.0lf
        to_log(LOG_INFO, "LLVM JIT: {length(funcs)} functions in {totalDt} sec{recompile_prog ? " (codegen)" : " (cache)"}\n")
    }
    return true
}
