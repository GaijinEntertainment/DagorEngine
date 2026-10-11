options gen2
options indenting = 4
options strict_smart_pointers = false
options stack = 4_194_304
options no_global_variables = false

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_dll_utils
require llvm/daslib/llvm_jit_plan public
require llvm/daslib/llvm_jit_cli
require llvm/daslib/llvm_exe
require llvm/daslib/jit_standalone
require daslib/c_api_header
require llvm/daslib/llvm_aot
require llvm/daslib/llvm_jit
require llvm/daslib/llvm_jit_common
require llvm/daslib/llvm_jit_di
require llvm/daslib/llvm_jit_intrin
require llvm/daslib/llvm_env

require daslib/ast_boost
require daslib/strings_boost
require daslib/defer

require daslib/fio
require daslib/option
require daslib/result
require jit

module llvm_jit_run shared private

//! JIT codegen driver shared by the `jit_llvm` simulate-macro (llvm_macro) and
//! the repo's `jit` tool. `run_jit` performs the whole compile->IR->optimize->
//! emit(.dll/.exe/.wasm) sequence for an already-simulated program+context, so
//! callers do not need to inject the `just_in_time` module at runtime.

var LINK_WHOLE_LIB = false // when true, standalone exe links against the whole library; when false, only against runtime

// Read by tests-cpp/small/test_jit_emitter_pin.cpp: FNV-1a64 of the emitter sources
// (normalized to LF; file list in the test)
let LLVM_JIT_EMITTER_HASH : uint64 = 0x1818a2b8f67bacfeul

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

// reopen the freshly written DLL for the install phase + GC stale artifacts - the ONE tail both
// the monolith and split arms share (the keep-mask decides which cached artifacts survive)
[macro_function]
def private reopen_and_gc_dll(var dll : DLLHandle?&; output_path : string; funcs : array<FunctionPtr>;
                              disabled : array<FunctionPtr>; var uids : UidNodes?; dll_hash_basename : string;
                              keep_objs : array<string>; mine : table<string>) {
    dll.reset()
    dll = get_dll_by_path(output_path)
    // linked ok but unloadable = eaten between the two; the panic text carries the diagnosis
    if (dll.handle == null) {
        panic("LLVM JIT: {output_path}.dll linked but cannot be loaded back - the file is missing or unreadable. On Windows check antivirus quarantine and exclude the .jitted_scripts directory from real-time scanning.\n")
    }
    let (missing, disabling) = get_dll_missing(funcs, disabled, dll, uids)
    if (!(missing |> empty()) || !(disabling |> empty())) {
        let names = join([for (mf in missing); "{mf.name}"], ", ")
        panic("LLVM JIT: reopened {output_path}.dll disagrees with what was just written - {length(missing)} function(s) missing or hash-mismatched ({names}), {length(disabling)} disabled-but-present\n")
    }
    if (!(dll_hash_basename |> empty())) {
        var keep_names : table<string>
        for (obj in keep_objs) {
            keep_names |> insert(base_name(obj))
        }
        jit_gc_stale_dlls(dir_name(output_path), dll_hash_basename, keep_names, mine)
    }
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
    di_finalize()
    // --jit-split-modules: drain the stashed partition states (the ambient one is handled below)
    for (p in g_split_parts) {
        dispose_split_part(p, p.ctx != g_ctx)
    }
    g_split_parts |> clear()
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

// "Debug Info Version" gates all !dbg metadata (absent -> silently stripped); "CodeView" flips
// COFF debug emission from DWARF to CodeView so lld-link /DEBUG can build PDB line tables
// (windows targets only). Per-module flags: split mode stamps every partition module.
def private add_debug_module_flags(debug_info : bool; target_triple : string) {
    return if (!debug_info)
    let div_key = "Debug Info Version"
    LLVMAddModuleFlag(g_mod, LLVMModuleFlagBehavior.Warning, div_key, uint64(long_length(div_key)),
        LLVMValueAsMetadata(LLVMConstInt(LLVMInt32TypeInContext(g_ctx), 3ul, 0)))
    // host_jit_triple() is "" on MSVC hosts — LLVM's default triple is the actual host answer
    let triple_owned = target_triple |> empty()
    let triple = triple_owned ? LLVMGetDefaultTargetTriple() : target_triple
    if (triple |> find("windows") != -1) {
        let cv_key = "CodeView"
        LLVMAddModuleFlag(g_mod, LLVMModuleFlagBehavior.Warning, cv_key, uint64(long_length(cv_key)),
            LLVMValueAsMetadata(LLVMConstInt(LLVMInt32TypeInContext(g_ctx), 1ul, 0)))
    }
    if (triple_owned) {
        LLVMDisposeMessage(triple)
    }
}

// irgen one function set into the current g_mod, with the per-function verifier panics
[macro_function]
def private irgen_functions(ctx : Context?; var uids : UidNodes?; attrs : Attributes?;
                            part : array<FunctionPtr>; jit_flags : LlvmJitFlags; jit_mode : LlvmJitMode;
                            prog : Program?) : bool {
    var has_externals = false
    for (fun in part) {
        has_externals = generate_llvm(prog, ctx, g_prim_t, uids, attrs, fun, jit_flags, jit_mode) || has_externals
        let fnmna = uids.get_dll_fn_name(fun)
        var fn_impl = LLVMGetNamedFunction(g_mod, fnmna.impl())
        var fn = LLVMGetNamedFunction(g_mod, fnmna.publ())
        assume maybe_mod = empty(prog.thisModuleName) ? "" : "in module {prog.thisModuleName}"
        if (!LLVMVerifyFunction(fn_impl, LLVMVerifierFailureAction.LLVMPrintMessageAction, false)) {
            reset_jit_globals_after_failure()
            panic("Internal jit error. Failed to get IR of '{fun.name} implementation' {maybe_mod}.\n")
        }
        if (!LLVMVerifyFunction(fn, LLVMVerifierFailureAction.LLVMPrintMessageAction, false)) {
            reset_jit_globals_after_failure()
            panic("Internal jit error. Failed to get IR of '{fun.name}' in module {maybe_mod}.\n")
        }
    }
    return has_externals
}

// whole-module verify + the g_failed panic tail (shared by the pre-optimize and post-optimize checks)
[macro_function]
def private verify_module_or_panic(funcs : array<FunctionPtr>) {
    if (!LLVMVerifyModule(g_mod, LLVMVerifierFailureAction.LLVMPrintMessageAction, false)) {
        g_failed = true
    }
    if (g_failed) {
        let fn_names = join([ for (fn in funcs); "{fn.name}" ], ",")
        reset_jit_globals_after_failure()
        panic("Internal jit error. Failed to get IR for functions {fn_names}.\n")
    }
}

// the IR dump debug rail: globaldce (drop unused declarations) then print
def private dump_module_ir(dump_to : string; mod : LLVMOpaqueModule?) {
    var pmo = LLVMCreatePassBuilderOptions()
    let dce_err = LLVMRunPasses(mod, "globaldce", null, pmo)
    if (dce_err != null) {
        let dce_msg = LLVMGetErrorMessage(dce_err)
        to_log(LOG_WARNING, "LLVM globaldce failed: {dce_msg}\n")
        LLVMDisposeErrorMessage(dce_msg)
    }
    LLVMDisposePassBuilderOptions(pmo)
    let module_str = LLVMPrintModuleToString(mod)
    let msg = clone_string(module_str)
    LLVMDisposeMessage(module_str)
    if (empty(dump_to)) {
        to_log(LOG_INFO, "LLVM JIT: {msg}\n");
        return
    }
    fopen(dump_to, "ab") $(f) {
        if (f != null) {
            f |> fwrite(msg)
        }
    }
}

// post-irgen tail: optimize, optional IR dump, whole-module verify
[macro_function]
def private optimize_and_verify_module(opt_level, size_level : int; use_host_cpu, dump_ir : bool;
                                       dump_to : string; funcs : array<FunctionPtr>; sanitize : JitSanitize) {
    optimize_llvm_module(opt_level |> uint(), size_level |> uint(), LLVM_INLINE_THRESHOLD, use_host_cpu, sanitize)
    if (dump_ir) {
        dump_module_ir(dump_to, g_mod)
    }
    verify_module_or_panic(funcs)
}

struct private JitSplitPart {
    ctx : LLVMContextRef
    mod : LLVMOpaqueModule?
    engine : LLVMExecutionEngineRef
    prim_t : PrimitiveTypes?
    tm : LLVMTargetMachineRef
    pbo : LLVMOpaquePassBuilderOptions?
    obj : string
    obj_final : string
    tag : string
}

def private publish_link_set(tmp_base, final_base : string) {
    let d = dir_name(tmp_base)
    let tmp_prefix = "{base_name(tmp_base)}."
    let final_name = base_name(final_base)
    var produced : array<string>
    dir(d) $(fname) {
        if (fname |> starts_with(tmp_prefix)) {
            produced |> push(fname)
        }
    }
    for (fname in produced) {
        let suffix = slice(fname, length(tmp_prefix), length(fname))
        publish_object("{d}/{fname}", "{d}/{final_name}.{suffix}")
    }
}

var g_published_names : table<string>

def private publish_object(tmp, final : string) {
    if (tmp == final || !stat(tmp).is_valid) return
    if (!rename(tmp, final)) {
        var err : string
        remove(tmp, err)
        if (!stat(final).is_valid) {
            to_log(LOG_WARNING, "LLVM JIT: could not publish {tmp} as {final}\n")
        }
        return
    }
    g_published_names |> insert(base_name(final))
}

// live split partitions between codegen and disposal - module-global so the failure path
// (reset_jit_globals_after_failure) can drain them and a bulk run leaks nothing
var g_split_parts : array<JitSplitPart>

def private dispose_split_part(var p : JitSplitPart; dispose_state : bool) {
    LLVMDisposeTargetMachine(p.tm)
    LLVMDisposePassBuilderOptions(p.pbo)
    if (dispose_state) {
        LLVMDisposeExecutionEngine(p.engine)   // engine owns p.mod
        LLVMContextDispose(p.ctx)
        unsafe {
            delete p.prim_t
        }
    }
}

//! what a split codegen knows of the artifact it feeds: the exe's entry and whether its partitions leave as bitcode
struct private SplitTarget {
    gen_exe : bool
    exe_main : string
    exe_strict : bool
    register_all_modules : bool
    lto : bool
}

//! the link set - the partition objects or bitcode files and the response file - is the link's alone
def private remove_split_link_files(objs : array<string>; output_path : string) {
    var err : string
    for (o in objs) {
        remove(o, err)
    }
    remove(split_link_rsp_path(output_path), err)
}

// nolint:STYLE014
// --jit-split-modules driver, three phases. A (serial das): partition `funcs` by owning das-module
// (program order), codegen each partition into its OWN (context, module, engine) emitter state -
// stashed in g_split_parts, the LAST left ambient for the shared install/finalize path. B: the
// jit_par_emit_run C++ pool optimizes + emits every partition on K threads (K=1 = serial bring-up;
// one LLVMContext per job is LLVM's threading contract; the SAME LLVMRunPasses/EmitToFile calls as
// the monolith path, entry points resolved through dasbind - libDaScript never links LLVM). C
// (serial das): post-optimize verify per partition, then dispose everything but the ambient state.
[macro_function, arch(at="../ARCHITECTURE_EXE.md#split-exe-lto")]
def private run_split_codegen(prog : Program?; ctx : Context?; funcs : array<FunctionPtr>;  // nolint:STYLE037,STYLE038 - the split orchestration is one linear pipeline (the cache probe adds its branches); the phases share the re-pointed g_* state
                              var uids : UidNodes?; jit_flags : LlvmJitFlags;
                              opt_level, size_level, split_threads : int; use_host_cpu : bool;
                              debug_info, dump_ir, obj_cache : bool; dump_to : string; target_triple : string;
                              output_path : string; log_jit_time : bool; sanitize : JitSanitize;
                              target : SplitTarget) : tuple<objs : array<string>; t_irgen : int; t_optemit : int; link_whole_lib : bool; main_ok : bool; partitions : int; cached : int> {
    var n_partitions = 0
    var n_cached = 0
    var phase_tm = ref_time_ticks()   // t_irgen covers partitioning + per-partition declare/irgen/ctors
    let publish_token = "{ref_time_ticks()}"
    // partition by owning das-module, first-appearance (= program module) order - deterministic
    var order : array<string>
    var buckets : table<string; array<FunctionPtr>>
    for (fn in funcs) {
        let mname = string(fn._module.name)
        if (!(buckets |> key_exists(mname))) {
            order |> push(mname)
        }
        buckets[mname] |> push(unsafe(reinterpret<Function?>(fn)))   // nolint:PERF006 - per-module counts unknown up front, one-shot build
    }
    let thrTag = split_threads < 0 ? "auto" : "{split_threads}"
    to_log(LOG_INFO, "LLVM JIT: split codegen (--jit-split-modules) - {length(order)} das-module partitions, {thrTag} threads -> one {target.gen_exe ? "exe" : "DLL"}\n")
    // the dir must exist before the FIRST emit (a missing dir crashes LLVMTargetMachineEmitToFile)
    mkdir_rec(dir_name(output_path))
    jit_par_emit_begin()
    g_split_parts |> reserve(length(order))
    // empty pipeline = pool skips optimization (the LLVM_ENABLE_OPT_PASS / LLVM_DEBUG_EVERYTHING rail)
    let pipeline = LLVM_ENABLE_OPT_PASS ? opt_pipeline_string(opt_level |> uint, size_level |> uint, sanitize) : ""
    // obj cache key = env salt + the RUNNING chained fold of module hashes: positional invalidation (ARCHITECTURE.md §2.1); probe = file existence on key-named objs
    var chain = LLVM_JIT_CODEGEN_VERSION
    let env_salt = jit_env_salt(opt_level, size_level, jit_flags.emit_prologue, debug_info, target_triple, true, use_host_cpu, sanitize)
    // DAS_JIT_DUMP_HASHES: per-function fold forensics - diff two runs to locate a key divergence
    let dump_hashes = g_env_jit.jit_dump_hashes
    var objs : array<string>
    objs |> reserve(length(order))
    var cached = 0
    var first_miss = true
    var link_whole_lib = false
    var main_ok = true
    let codegen_level = target.gen_exe ? 3u : uint(opt_level)   //! an exe pins codegen level 3 like the one-unit form (a shipped artifact); the DLL split follows the dev rail's level
    for (pname, pi in order, count()) {
        let ptag = empty(pname) ? "main" : pname
        for (fun in buckets[pname]) {
            if (dump_hashes) {
                to_log(LOG_INFO, "HASHDUMP {ptag} {get_mangled_name(fun)} {get_function_aot_hash(fun)}\n")
            }
            chain = (chain ^ hash(get_mangled_name(fun))) * JIT_FNV_PRIME   // a rename must re-key: the symbol lives in the .o
            chain = (chain ^ get_function_aot_hash(fun)) * JIT_FNV_PRIME
        }
        chain = fold_loop_hints(buckets[pname], chain)
        chain = fold_llvm_code_hints(buckets[pname], chain)
        chain = fold_function_hints(buckets[pname], chain)
        let pkey = (chain ^ env_salt) * JIT_FNV_PRIME
        // cache off (incl. a pinned output path, where no GC may run): positional names, overwritten in place - key-named objs would accumulate without bound there
        let obj_final = (obj_cache ? artifact_path("{dir_name(output_path)}/{pkey}.m_{ptag}", JitArtifact.object)
                             : artifact_path("{output_path}.p{pi}_{ptag}", JitArtifact.object))
        let obj = "{obj_final}.{publish_token}.tmp"
        objs |> push(obj_final)   // link order = partition order, hit or miss
        if (obj_cache && stat(obj_final).is_valid) {
            cached++
            if (log_jit_time) {
                to_log(LOG_INFO, "LLVM JIT: obj cache hit {ptag}\n")
            }
            continue
        }
        if (!first_miss) {
            // the FIRST miss consumes the ambient init_jit state (a re-init would leak it); earlier misses' state lives on in g_split_parts until phase C
            init_jit_state(opt_level |> uint, target_triple, use_host_cpu, "llvm_jit_{ptag}")
            add_debug_module_flags(debug_info, target_triple)
        } else {
            // ambient module identifier must match a later-miss emission - same key, same bytes
            let mod_ident = "llvm_jit_{ptag}"
            LLVMSetModuleIdentifier(g_mod, mod_ident, uint64(long_length(mod_ident)))
        }
        first_miss = false
        let attrs = new Attributes(g_ctx)
        // eager declare covers OWN pairs only; out-of-partition callees declare-on-reference
        var fake_visitor = new LlvmJitVisitor(prog, ctx, g_prim_t, uids, jit_flags, LlvmJitMode.JIT, null, attrs)
        ast_gc_guard() {
            for (fun in buckets[pname]) {
                fake_visitor->add_llvm_functions(fun)
            }
        }
        unsafe {
            delete fake_visitor
        }
        // the has_externals return is meaningful only on the wasm path, unreachable under split - discard
        irgen_functions(ctx, uids, attrs, buckets[pname], jit_flags, LlvmJitMode.JIT, prog)
        di_finalize()
        verify_module_or_panic(buckets[pname])
        // fileinfo ctors ride llvm.global_ctors (private; the link appends all, LoadLibrary runs every one)
        generate_global_ctors_dtors(g_prim_t, false)
        if (g_jit_fast_math) {
            apply_fast_math_to_module(g_mod)
        }
        g_split_parts |> push(JitSplitPart(ctx = g_ctx, mod = g_mod, engine = g_engine, prim_t = g_prim_t,
            tm = create_default_target_machine(codegen_level, use_host_cpu),
            pbo = create_jit_pass_builder_options(LLVM_INLINE_THRESHOLD),
            obj = obj, obj_final = obj_final, tag = ptag))
    }
    if (target.gen_exe) {
        let ptag = "exe_main"
        if (!first_miss) {
            init_jit_state(opt_level |> uint, target_triple, use_host_cpu, "llvm_jit_{ptag}")
            add_debug_module_flags(debug_info, target_triple)
        }
        first_miss = false
        let attrs = new Attributes(g_ctx)
        var fake_visitor = new LlvmJitVisitor(prog, ctx, g_prim_t, uids, jit_flags, LlvmJitMode.JIT, null, attrs)
        ast_gc_guard() {
            for (fun in funcs) {   // every pair declared: the entry looks its functions up by name, and the link resolves each to its partition
                fake_visitor->add_llvm_functions(fun)
            }
        }
        unsafe {
            delete fake_visitor
        }
        for (glob in keys(g_split_glob_names)) {
            let decl = LLVMAddGlobal(g_mod, g_prim_t.LLVMVoidPtrType(), glob)
            LLVMSetLinkage(decl, LLVMLinkage.LLVMExternalLinkage)
        }
        let res = inject_main(ctx, g_ctx, prog, target.exe_main, g_mod, g_prim_t, uids, target.exe_strict, target.register_all_modules)
        link_whole_lib = res.link_whole_lib
        main_ok = res.fn != null
        di_finalize()
        if (main_ok) {
            let no_funcs : array<FunctionPtr>
            verify_module_or_panic(no_funcs)
            generate_global_ctors_dtors(g_prim_t, false)
            if (g_jit_fast_math) {
                apply_fast_math_to_module(g_mod)
            }
            let obj_final = artifact_path("{output_path}.p{length(order)}_{ptag}", JitArtifact.object)
            let obj = "{obj_final}.{publish_token}.tmp"
            objs |> push(obj_final)
            g_split_parts |> push(JitSplitPart(ctx = g_ctx, mod = g_mod, engine = g_engine, prim_t = g_prim_t,
                tm = create_default_target_machine(codegen_level, use_host_cpu),
                pbo = create_jit_pass_builder_options(LLVM_INLINE_THRESHOLD),
                obj = obj, obj_final = obj_final, tag = ptag))
        }
    }
    if (obj_cache) {
        n_partitions = length(order)
        n_cached = cached
        to_log(LOG_INFO, "LLVM JIT: obj cache - {cached}/{length(order)} partitions cached\n")
    } elif (!target.gen_exe) {   // an exe has no cache to be off
        // the off-state changes what the run compiles (every partition re-emits) - announce it
        to_log(LOG_INFO, "LLVM JIT: obj cache off - all {length(order)} partitions re-emit\n")
    }
    let lto_run = main_ok && (target.lto || (g_env_jit.jit_probe_lto && !target.gen_exe))   //! the probe is the DLL rail's: an exe's link takes its LTO flags from --jit-lto alone
    if (main_ok) {
        for (p in g_split_parts) {
            stamp_module_target(p.mod, p.tm)
            if (sanitize.address) {
                mark_sanitize_address(p.mod)
            }
            jit_par_emit_add(p.mod, p.tm, p.pbo, pipeline, lto_run ? "" : p.obj)   // an LTO partition leaves as bitcode: the pool optimizes it and emits no object
        }
    }
    let tIr = get_time_usec(phase_tm)
    phase_tm = ref_time_ticks()
    // phase B - per-job walls logged by the pool under log_compile_time; all partitions cached -> zero jobs, skip the pool (and the LLVM symbol resolution) outright
    if (main_ok && !empty(g_split_parts)) {
        let llvm_lib = __dasbind_get_library("LLVM.dll")
        if (llvm_lib == null) {
            reset_jit_globals_after_failure()
            panic("LLVM JIT: split emit - can't resolve the LLVM library handle")
        }
        // resolve once + validate ALL entry points - a mismatched LLVM.dll names its missing symbol here
        let lfn_names = ["LLVMRunPasses", "LLVMVerifyModule", "LLVMTargetMachineEmitToFile",
                         "LLVMGetErrorMessage", "LLVMDisposeErrorMessage", "LLVMDisposeMessage"]
        var lfns : array<void?>
        lfns |> reserve(length(lfn_names))
        for (nm in lfn_names) {
            let p = llvm_lib |> get_function_address(nm)
            if (p == null) {
                reset_jit_globals_after_failure()
                panic("LLVM JIT: split emit - can't resolve {nm} in the LLVM library")
            }
            lfns |> push(p)
        }
        let ok = jit_par_emit_run(split_threads, int(LLVMCodeGenFileType.LLVMObjectFile),
            lfns[0], lfns[1], lfns[2], lfns[3], lfns[4], lfns[5], log_jit_time)
        if (!ok) {
            reset_jit_globals_after_failure()
            panic("LLVM JIT: split parallel optimize+emit failed")
        }
        for (p in g_split_parts) {
            publish_object(p.obj, p.obj_final)
        }
    }
    // phase C - the opt-in IR dump only (workers verified each module post-optimize, pre-emit; cached partitions have no live IR)
    if (dump_ir) {
        for (p in g_split_parts) {
            dump_module_ir(dump_to, p.mod)
        }
    }
    // --jit-lto (a split exe) and DAS_JIT_PROBE_LTO (the DLL dev probe; the env folds into the cache keys; ARCHITECTURE.md §3): bitcode partitions + an LTO link
    if (lto_run) {
        // enforce the documented precondition outright - probe behavior must not depend on prior cache state
        if (obj_cache || cached != 0) {
            reset_jit_globals_after_failure()
            panic("LLVM JIT: {target.lto ? "--jit-lto" : "DAS_JIT_PROBE_LTO"} needs every partition live - rerun with --jit-obj-cache=0")
        }
        assert(length(g_split_parts) == length(objs))   // objs[i] <-> g_split_parts[i] holds only with nothing cached
        if (target.lto) {
            to_log(LOG_INFO, "LLVM JIT: --jit-lto - {length(g_split_parts)} bitcode partitions, the link runs LTO\n")
        } else {
            to_log(LOG_WARNING, "LLVM JIT: DAS_JIT_PROBE_LTO - linking {length(g_split_parts)} bitcode partitions (LTO probe artifact)\n")
        }
        let host_triple = host_jit_triple()   //! the daslang host's own triple where it differs from LLVM's default (mingw), the source create_default_target_machine reads
        let triple_owned = empty(target_triple) && empty(host_triple)
        let lto_triple = triple_owned ? LLVMGetDefaultTargetTriple() : (empty(target_triple) ? host_triple : target_triple)
        let machine = default_target_cpu_features(use_host_cpu)
        for (p, i in g_split_parts, count()) {
            LLVMSetTarget(p.mod, lto_triple)   // lld's LTO backend reads the MODULE triple (normal emission gets it from the TargetMachine)
            stamp_target_attrs(p.mod, machine.cpu, machine.features)
            let bc = split_part_bitcode_path(p.obj)
            if (LLVMWriteBitcodeToFile(p.mod, bc) != 0) {
                reset_jit_globals_after_failure()
                panic("LLVM JIT: LTO probe - can't write bitcode {bc}")
            }
            objs[i] = bc
        }
        if (triple_owned) {
            LLVMDisposeMessage(lto_triple)
        }
    }
    for (p in g_split_parts) {
        dispose_split_part(p, p.ctx != g_ctx)   // the ambient (last) survives for install/finalize
    }
    g_split_parts |> clear()
    // captured AFTER phase C: verifies, dump, and teardown land inside "optimize" - no untimed gap
    let tOptEmit = get_time_usec(phase_tm)
    return <- (objs <- objs, t_irgen = tIr, t_optemit = tOptEmit, link_whole_lib = link_whole_lib, main_ok = main_ok, partitions = n_partitions, cached = n_cached)
}

struct public JitRun {
    ok : bool
    partitions : int
    cached : int
    output_path : string
    compile_only : bool
    use_host_cpu : bool
}

//! Run the full JIT backend for an already-compiled & simulated program: take the plan, pass its
//! candidates through the content gate, generate and optimize IR, and emit a .dll / .exe / .wasm
//! per the program's jit policies. `llvm_jit_link` calls this on a DLL cache miss.
[macro_function]
def public run_jit(prog : Program?; var ctx : Context?) : JitRun {
    let r <- parse_args(type<JitCliOptions>)
    return run_jit(prog, ctx, r |> unwrap_or_default)
}

[macro_function]
def public run_jit(prog : Program?; var ctx : Context?; cli_opts : JitCliOptions) : JitRun {  // nolint:STYLE037,STYLE038 - the JIT backend driver: one linear codegen pipeline over the plan; the phases share the module/target state and do not survive a split
    var run = JitRun(ok = true)
    if (prog.policies.aot && !prog.policies.jit_emit_object && !prog.policies.jit_enabled) {
        return run
    }
    let totalTime = ref_time_ticks()
    var phase_tm = ref_time_ticks()
    var plan <- make_jit_plan(prog, ctx, false, cli_opts)
    if (report_collect_errors(plan)) {
        run.ok = false
        return run
    }
    assume aot_host = plan.aot_host
    assume jit_all_functions = plan.jit_all_functions
    assume debug_info = plan.debug_info
    assume dump_ir = plan.dump_ir
    assume gen_exe = plan.gen_exe
    assume gen_lib = plan.gen_lib
    assume lib_static = plan.lib_static
    assume lib_bindings = plan.lib_bindings
    assume exe_main = plan.exe_main
    assume compile_only = plan.compile_only
    run.compile_only = compile_only
    assume emit_aot_object = plan.emit_aot_object
    assume use_dll = plan.use_dll
    assume split_modules = plan.split_modules
    assume jit_split = plan.jit_split
    assume obj_cache = plan.obj_cache
    assume jit_lto = plan.jit_lto
    assume register_all_modules = plan.register_all_modules
    assume opt_level = plan.opt_level
    assume size_level = plan.size_level
    assume emit_prologue = plan.emit_prologue
    assume exe_strict = plan.exe_strict
    assume config_out_path = plan.config_out_path
    assume output_path = plan.output_path
    run.output_path = output_path
    assume path_to_shared_lib = plan.path_to_shared_lib
    assume path_to_linker = plan.path_to_linker
    assume target_triple = plan.target_triple
    assume gen_wasm = plan.gen_wasm
    assume runtime_lib_override = plan.runtime_lib_override
    assume emit_object = plan.emit_object
    assume linker_string = plan.linker_string
    assume exe_host_cpu = plan.exe_host_cpu
    assume use_host_cpu = plan.use_host_cpu
    run.use_host_cpu = use_host_cpu
    assume log_jit_time = plan.log_jit_time
    assume dll_hash_basename = plan.dll_hash_basename
    assume disabled = plan.disabled
    // Drives the $::is_standalone_exe JIT intrinsic in llvm_jit_intrin so daslang
    // code can constant-fold to true when this codegen run produces a standalone exe.
    llvm_jit_intrin::g_jit_exe_mode = gen_exe
    defer() { llvm_jit_intrin::g_jit_exe_mode = false; }
    // Emit builtin-accessor globals internal + null in object mode (re-resolved by
    // the emit_aot_object_globinit ctor); reset after this run.
    g_emit_aot_object_globals = emit_aot_object
    defer() { g_emit_aot_object_globals = false; }
    g_jit_split_active = jit_split
    defer() { g_jit_split_active = false; }
    g_jit_split_exe = jit_split && gen_exe
    g_split_glob_names |> clear()   // collection starts with the flag: a slot minted before the partitions would otherwise miss the entry's declare loop
    defer() {
        g_jit_split_exe = false
        g_split_glob_names |> clear()
    }
    // --jit-threads: set the codegen global before any IR is built so both the data-layout machine
    // and write_wasm's emission machine pick up the threaded feature set.
    g_target_wasm_threads = plan.wasm_threads
    // --jit-check-abi: set before codegen so llvm_exe's sweep sees the flag.
    g_jit_check_handled_abi = plan.check_abi
    // the disable pass consults target-gated intrinsic tables (has_intrinsic) — the
    // g_target_* truth must be set BEFORE it runs, not first at init_jit time below.
    init_jit_target_flags(target_triple, use_host_cpu)
    var funcs : array<FunctionPtr>
    var visitorDisabled = 0
    var emit_failed = false
    defer() { reset_codegen_accumulators(); }
    var disableJitVisitor = new DisableJitVisitor()
    make_visitor(*disableJitVisitor) $(disableJitVisitorAdapter) {
        for (fun in plan.candidates) {
            disableJitVisitor.disable = false
            // if DisableJitVisitor ever grows a preVisitFunction check again, it must
            // be dispatched explicitly here — `visit(fun)` never calls preVisitFunction
            fun |> visit(disableJitVisitorAdapter)
            if (!disableJitVisitor.disable) {
                funcs |> push(fun)
            } else {
                visitorDisabled++
                to_log(LOG_INFO, "LLVM JIT: disabled {fun.name}\n")
            }
        }
    }
    unsafe {
        delete disableJitVisitor
    }
    mark_jit_selection(prog, funcs)
    // emit_aot_object always writes an object even with no functions to emit (a fully-no_aot
    // module): an empty .o with just the fileinfo ctor, so the build always has the file it
    // expects and those functions interpret at load (as Program::linkCppAot skips them).
    if (!empty(funcs) || emit_aot_object || gen_lib) {
        // the plan's fold over the candidates - the first aot-hash per function lands there (cached after)
        let t_hash = get_time_usec(phase_tm)
        phase_tm = ref_time_ticks()
        var t_declare = 0
        var t_probe = 0
        var t_irgen = 0
        var t_opt = 0
        var t_emit = 0
        var t_install = 0
        // Pass target_triple so init_jit can pin the module data layout
        // BEFORE IR construction (wasm32 cross-compile needs 4-byte pointer
        // sizes baked into GEP folding from the very first IR instruction).
        init_jit(opt_level |> uint, target_triple, use_host_cpu)
        let t_init = get_time_usec(phase_tm)

        add_debug_module_flags(debug_info, target_triple)

        var jit_flags : LlvmJitFlags
        jit_flags.emit_prologue = emit_prologue
        jit_flags.need_di = debug_info
        let jit_mode = emit_aot_object ? LlvmJitMode.LLVM_AOT : (gen_exe ? LlvmJitMode.EXE : LlvmJitMode.JIT)

        let attrs = new Attributes(g_ctx)
        var uids = create_uid_nodes(prog, funcs, gen_exe)
        // split mode declares per partition inside run_split_codegen (eager-declaring all
        // functions into every partition module would be O(funcs x partitions) dead work)
        if (!jit_split) {
            var fake_visitor = new LlvmJitVisitor(prog, ctx, g_prim_t, uids, jit_flags, jit_mode, null, attrs)
            ast_gc_guard() {
                for (fun in funcs) {
                    fake_visitor->add_llvm_functions(fun)
                }
            }
            unsafe {
                delete fake_visitor
            }
        }
        t_declare = get_time_usec(phase_tm) - t_init
        phase_tm = ref_time_ticks()
        var g_dynamic_lib_handle : DLLHandle? = null;
        var recompile_prog = true
        if (use_dll) {
            g_dynamic_lib_handle = get_dll_by_path(output_path)
            let (missing, disabling) = get_dll_missing(funcs, disabled, g_dynamic_lib_handle, uids)
            let dll_changed = !(missing |> empty() && disabling |> empty())
            recompile_prog = dll_changed
            if (recompile_prog) {
                g_dynamic_lib_handle.reset()
                to_log(LOG_INFO, "LLVM JIT: DLL cache miss, codegen for {output_path}.dll\n")
            } else {
                to_log(LOG_INFO, "LLVM JIT: DLL cache hit {output_path}.dll\n")
            }
        }
        t_probe = get_time_usec(phase_tm)
        phase_tm = ref_time_ticks()
        var jit_has_externals = false
        var split_parts = 0
        if (recompile_prog && jit_split) {
            let sres <- run_split_codegen(prog, ctx, funcs, uids, jit_flags, opt_level, size_level,
                split_modules, use_host_cpu, debug_info, dump_ir, obj_cache, plan.dump_ir_path, target_triple, output_path, log_jit_time, plan.sanitize,
                SplitTarget(gen_exe = gen_exe, exe_main = exe_main, exe_strict = exe_strict, register_all_modules = register_all_modules, lto = jit_lto))
            let link_base = "{output_path}.{ref_time_ticks()}"
            t_irgen = sres.t_irgen
            // the pool interleaves passes and emission, so "optimize" carries the combined
            // optimize + emit-obj + post-opt-verify + partition-teardown wall, and "emit+link"
            // below is the link+reopen tail only (ARCHITECTURE.md §1.1 split mapping)
            t_opt = sres.t_optemit
            run.partitions = sres.partitions
            run.cached = sres.cached
            split_parts = length(sres.objs)
            phase_tm = ref_time_ticks()
            if (gen_exe) {
                if (!sres.main_ok) {
                    reset_codegen_accumulators()
                    finalize_jit(use_dll, gen_exe, g_dynamic_lib_handle)
                    to_log(LOG_ERROR, "LLVM EXE: no executable written for {output_path}\n")
                    run.ok = false
                    return run
                }
                LINK_WHOLE_LIB = sres.link_whole_lib
                if (link_from_objects(sres.objs, output_path, JitArtifact.exe, path_to_shared_lib, path_to_linker, linker_string, LINK_WHOLE_LIB, debug_info, log_jit_time, jit_lto, 3u)) {   // under LTO the linker is the codegen, at the level every exe pins
                    remove_split_link_files(sres.objs, output_path)
                } else {
                    remove_split_link_files(sres.objs, output_path)   // the partition set is the link's alone: a failed link leaves nothing beside the user's -output
                    emit_failed = true
                }
            } elif (!link_from_objects(sres.objs, link_base, JitArtifact.jit_dll, path_to_shared_lib, path_to_linker, linker_string, LINK_WHOLE_LIB, debug_info, log_jit_time)) {
                // self-heal: drop the whole link set before the panic - a truncated .o from a
                // killed emit would otherwise probe-hit (and re-fail this link) forever
                for (o in sres.objs) {
                    var oerr : string
                    remove(o, oerr)
                }
                reset_jit_globals_after_failure()
                panic("LLVM JIT: split link failed for {output_path} - cached partition objects dropped, the next run re-emits them")
            }
            if (!gen_exe) {
                publish_link_set(link_base, output_path)
                // partition objects are named by partition key, not the dll hash - the GC keep-set
                // preserves exactly the current link set. The LTO probe's artifact set is one-off:
                // skip its GC so a probe run cannot evict the normal cache generation.
                let lto_probe = g_env_jit.jit_probe_lto
                reopen_and_gc_dll(g_dynamic_lib_handle, output_path, funcs, disabled, uids, lto_probe ? "" : dll_hash_basename, sres.objs, g_published_names)
            }
        } elif (recompile_prog) {
            jit_has_externals = irgen_functions(ctx, uids, attrs, funcs, jit_flags, jit_mode, prog)
            if (gen_lib) {
                let export_marked = plan.lib_export_marked
                let res = inject_lib(ctx, g_ctx, prog, g_mod, g_prim_t, uids, exe_strict,
                    prog.policies.export_public_functions || export_marked, output_path,
                    plan.register_all_modules)
                LINK_WHOLE_LIB = res.link_whole_lib
                if (res.fn == null) {
                    finalize_jit(use_dll, gen_exe, g_dynamic_lib_handle)
                    to_log(LOG_ERROR, "LLVM LIB: no library written for {output_path}\n")
                    run.ok = false
                    return run
                }
            } elif (gen_exe) {
                let res = inject_main(ctx, g_ctx, prog, exe_main, g_mod, g_prim_t, uids, exe_strict,
                    plan.register_all_modules)
                LINK_WHOLE_LIB = res.link_whole_lib
                if (res.fn == null) {
                    // before finalize_jit, which disposes the engine that owns g_mod
                    di_finalize()
                    reset_codegen_accumulators()
                    finalize_jit(use_dll, gen_exe, g_dynamic_lib_handle)
                    to_log(LOG_ERROR, "LLVM EXE: no executable written for {output_path}\n")
                    run.ok = false
                    return run
                }
            }
            di_finalize()
            verify_module_or_panic(funcs)
            // Offline AOT object: build the self-registration + glob-re-resolution ctors (llvm_aot pass).
            var aot_ctor : LLVMOpaqueValue? = null
            var aot_dtor : LLVMOpaqueValue? = null
            if (emit_aot_object) {
                var pair = build_llvm_aot_ctors(g_ctx, g_mod, g_prim_t, funcs, uids, prog)
                aot_ctor = pair._0
                aot_dtor = pair._1
                emit_aot_pull_anchor(g_mod, g_prim_t, base_name(output_path))
            }
            generate_global_ctors_dtors(g_prim_t, emit_aot_object ? false : !(use_dll || gen_exe), aot_ctor, aot_dtor)
            if (emit_aot_object) {
                let plat = get_platform_name()
                if (plat == "linux" || plat == "android") {
                    move_global_ctors_to_init_array(g_mod, g_prim_t, base_name(output_path))
                }
                // before optimization: the folding this catches also deletes the evidence
                let unresolved = collect_unresolved_address_globals(g_mod)
                if (!(unresolved |> empty())) {
                    reset_jit_globals_after_failure()
                    panic("Internal jit error. AOT address globals left unresolved by the globinit ctor: {join(unresolved, ", ")}\n")
                }
            }
            if (g_jit_fast_math) {
                apply_fast_math_to_module(g_mod)   // EXPERIMENT: stamp all FP ops fast (non-bit-exact ceiling)
            }
            if (use_host_cpu && !(compile_only || emit_aot_object || gen_lib || gen_wasm || gen_exe || use_dll)) {
                stamp_host_target_attrs(g_mod)   // the module runs on the in-memory engine, which carries no cpu or features of its own
            }
            t_irgen = get_time_usec(phase_tm)
            phase_tm = ref_time_ticks()
            optimize_and_verify_module(opt_level, size_level, use_host_cpu, dump_ir, plan.dump_ir_path, funcs, plan.sanitize)
            // optimize covers the passes plus the opt-in IR dump and the post-opt verify
            t_opt = get_time_usec(phase_tm)
            phase_tm = ref_time_ticks()
            if (compile_only) {
                to_log(LOG_INFO, "LLVM JIT: compile-only - module built, optimized and verified; no artifact written, nothing installed\n")
            } elif (emit_aot_object) {
                emit_failed ||= !emit_object_only(g_mod, output_path, use_host_cpu)
            } elif (gen_lib) {
                mkdir_rec(dir_name(output_path))
                emit_failed ||= !write_artifact(g_mod, output_path, lib_static ? JitArtifact.static_lib : JitArtifact.shared_lib,
                    path_to_shared_lib, path_to_linker, linker_string, LINK_WHOLE_LIB, exe_host_cpu,
                    debug_info, log_jit_time)
                let lib_names = CNames(prefix = lib_prefix_from_path(output_path), this_module = prog.getThisModule)
                if (!emit_c_header(g_lib_exports, lib_names, string(prog.getThisModule.fileName),
                        output_path, lib_link_note(lib_static, LINK_WHOLE_LIB))) {
                    to_log(LOG_ERROR, "LLVM LIB: the library is written but its C header is not - {output_path}.h\n")
                    emit_failed = true
                }
                if (!(lib_bindings |> empty())) {
                    emit_failed ||= !write_das_bindings(g_lib_exports, lib_names, string(prog.getThisModule.fileName),
                        output_path, lib_bindings, lib_static)
                }
            } elif (gen_wasm) {
                mkdir_rec(dir_name(output_path))
                // path_to_linker is reused as the optional emcc override
                // for the wasm link (mirrors how it's used for the host
                // linker, lld-link/clang/c++, on the host path).
                emit_failed ||= !write_wasm(g_mod, output_path, target_triple, path_to_linker, jit_has_externals, runtime_lib_override, emit_object)
            } elif (gen_exe) {
                mkdir_rec(dir_name(output_path))
                emit_failed ||= !write_artifact(g_mod, output_path, JitArtifact.exe, path_to_shared_lib, path_to_linker,
                    linker_string, LINK_WHOLE_LIB, exe_host_cpu, debug_info)
            } elif (use_dll) {
                mkdir_rec(dir_name(output_path))
                emit_failed ||= !write_artifact(g_mod, output_path, JitArtifact.jit_dll, path_to_shared_lib, path_to_linker,
                    linker_string, LINK_WHOLE_LIB, true, debug_info, log_jit_time, opt_level |> uint)
                let no_keep_objs : array<string>   // monolith artifacts all carry the dll-hash prefix
                reopen_and_gc_dll(g_dynamic_lib_handle, output_path, funcs, disabled, uids, dll_hash_basename, no_keep_objs, g_published_names)
            }
        }
        t_emit = get_time_usec(phase_tm)
        phase_tm = ref_time_ticks()
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
            jit_log_report(funcs, true, opt_level, true, false, 0, debug_info, log_jit_time, totalTime,
                t_hash, t_init, t_declare, t_probe, t_irgen, t_opt, t_emit, 0, 0)
            run.ok = !emit_failed
            return run
        }
        if (use_dll) {
            assert(g_dynamic_lib_handle.handle != null)
            let extern_errors <- resolve_dll_externs(prog, g_dynamic_lib_handle, ctx, uids, funcs)
            for (e in extern_errors) {
                failed(e)
            }
        }
        if (!gen_exe && !compile_only) {
            if (use_dll) {
                install_dll_functions(g_dynamic_lib_handle, ctx, uids, funcs)
            } else {
                for (fun in funcs) {
                    let simfun = get_function_by_mangled_name_hash(hash(get_mangled_name(fun)), *ctx)
                    var gen = generate_llvm_code(g_dynamic_lib_handle, uids.get_dll_fn_name(fun), false)
                    if (gen != null) {
                        gen |> unsafe(instrument_jit(simfun, fun.at, *ctx))
                    } else {
                        unsafe(remove_jit(simfun))
                    }
                }
            }
        }
        t_install = get_time_usec(phase_tm)
        phase_tm = ref_time_ticks()
        // compile-only takes the exe-style teardown: dispose the engine/context outright,
        // never RunStaticConstructors on (or install from) a module that may target a
        // foreign arch.
        var result = finalize_jit(use_dll, gen_exe || compile_only, g_dynamic_lib_handle)

        set_jit_state(*ctx, result.dll, result.ee, result.ctx)
        let t_finalize = get_time_usec(phase_tm)

        // a pinned jit_output_path cache probe compares function hashes only (not opt level),
        // so the tier label is asserted only when this run built the DLL or the content-addressed
        // name (which folds opt_level) matched
        let tier_known = recompile_prog || !empty(dll_hash_basename)
        jit_log_report(funcs, recompile_prog, opt_level, tier_known, jit_split, split_parts, debug_info, log_jit_time, totalTime,
            t_hash, t_init, t_declare, t_probe, t_irgen, t_opt, t_emit, t_install, t_finalize)
    } else {
        to_log(LOG_INFO, "LLVM JIT: 0 functions to jit ({length(disabled)} marked no_jit, {visitorDisabled} disabled by content) - the WHOLE program runs interpreted\n")
    }
    run.ok = !emit_failed
    return run
}
