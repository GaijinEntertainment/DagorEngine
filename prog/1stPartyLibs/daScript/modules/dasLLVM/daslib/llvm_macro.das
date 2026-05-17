options gen2
options indenting = 4
options strict_smart_pointers = false
options stack = 4_194_304

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_dll_utils
require llvm/daslib/llvm_exe
require llvm/daslib/llvm_jit
require llvm/daslib/llvm_jit_common
require llvm/daslib/llvm_jit_intrin

require daslib/ast_boost
require daslib/strings_boost
require daslib/defer

require daslib/fio
require daslib/clargs
require jit

module llvm_macro shared private

var LINK_WHOLE_LIB = false // when true, standalone exe links against the whole library; when false, only against runtime

// Bump this constant whenever LLVM codegen / linking logic changes in a way that
// invalidates cached DLLs (e.g. edits to llvm_jit.das, llvm_macro.das, llvm_jit_common.das,
// runtime helper ABI, default target triple). Cache filenames fold this in, so a bump
// makes every previously written DLL miss the cache on the next run and get GC'd.
let LLVM_JIT_CODEGEN_VERSION : uint64 = 0x01ul

let JIT_FNV_PRIME : uint64 = 1099511628211ul

def jit_dll_basename(funcs : array<FunctionPtr>; opt_level : int; size_level : int;
                     emit_prologue : bool; debug_info : bool) : string {
    var h = LLVM_JIT_CODEGEN_VERSION
    for (fn in funcs) {
        h = (h ^ get_function_aot_hash(fn)) * JIT_FNV_PRIME
    }
    h = (h ^ uint64(opt_level)) * JIT_FNV_PRIME
    h = (h ^ uint64(size_level)) * JIT_FNV_PRIME
    h = (h ^ (emit_prologue ? 1ul : 0ul)) * JIT_FNV_PRIME
    h = (h ^ (debug_info ? 1ul : 0ul)) * JIT_FNV_PRIME
    return "{h}"
}

def jit_gc_stale_dlls(subdir : string; keep_basename : string) {
    // Delete any file in subdir whose basename does not match the current hash.
    // Covers stale `.dll` plus linker artifacts (`.o`, `.lib`, `.exp`, `.pdb`).
    let keep_prefix = "{keep_basename}."
    dir(subdir) $(fname) {
        if (fname == "." || fname == "..") {
            return
        }
        if (fname |> starts_with(keep_prefix)) {
            return
        }
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

// This function should not be calling anything not marked with `no_jit`
[finalize(late), no_jit]
def public free_jit_context {
    // Free everything only if it's the last one context.
    if (get_context_share_counter() == 1ul) {
        get_jit_state() $(dll, ee, ctx : void?) {
            if (dll != null) {
                close_dynamic_library(dll)
            } elif (ee != null || ctx != null) {
                assert(ee != null && ctx != null)
                ee |> LLVMRunStaticDestructors()
                LLVMDisposeExecutionEngine(unsafe(reinterpret<LLVMExecutionEngineRef>(ee)))
                LLVMContextDispose(unsafe(reinterpret<LLVMContextRef>(ctx)))
            }
        }
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


[CommandLineArgs]
struct JitCliOptions {
    @clarg_name = "jit-opt-level"
    @clarg_doc = "JIT: LLVM optimization level (0..3); 0 disables all passes"
    opt_level : Option<int>

    @clarg_name = "jit-size-level"
    @clarg_doc = "JIT: LLVM size-optimization level (0..2)"
    size_level : Option<int>

    @clarg_short = "g"
    @clarg_name = "jit-debug"
    @clarg_doc = "JIT: emit debug info / symbols"
    debug_info : Option<bool>

    @clarg_short = "d"
    @clarg_name = "jit-dump"
    @clarg_doc = "JIT: dump generated LLVM IR"
    dump_ir : Option<bool>
}


[simulate_macro(name="jit_llvm")]
class JIT_LLVM : AstSimulateMacro {
    def override simulate(prog : Program?; var ctx : Context?) : bool {
        return true if (is_in_completion() || is_compiling_macros() || is_folding())
        var funcs : array<FunctionPtr>
        var disabled : array<FunctionPtr>
        var disableJitVisitor = new DisableJitVisitor()

        var cli_opts = JitCliOptions()
        let _ = parse_args(cli_opts)
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
        let use_dll = prog.policies.jit_dll_mode && das_is_dll_build() && !gen_exe
        let opt_level = cli_opts.opt_level |> unwrap_or(prog.policies.jit_opt_level)
        let size_level = cli_opts.size_level |> unwrap_or(prog.policies.jit_size_level)
        let emit_prologue = prog.policies.emit_prologue
        // If you're sure `dynamic` functions is not reachable set this flag
        // to `true` and fail in runtime on `jit_trap` if they turns
        // out to be reachable.
        let exe_strict = true
        assume config_out_path = string(prog.policies.jit_output_path)
        var output_path = config_out_path |> empty() ? ".jitted_scripts/{prog.thisNamespace}" : config_out_path
        assume path_to_shared_lib = string(prog.policies.jit_path_to_shared_lib)
        assume path_to_linker = string(prog.policies.jit_path_to_linker)

        // Set global options
        LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS = prog._options |> find_arg("jit_vec3_ldu") ?as tBool ?? LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS_DEFAULT

        make_visitor(*disableJitVisitor) $(disableJitVisitorAdapter) {
            prog |> for_each_module() $(mod) {
                mod |> for_each_function("") $(fun) {
                    let rqj = fun.moreFlags.requestJit
                    fun.moreFlags &= ~MoreFunctionFlags.requestJit
                    if (fun.flags.used && (jit_all_functions || rqj)) {
                        if (!fun.moreFlags.requestNoJit) {
                            disableJitVisitor.disable = false
                            fun |> visit(disableJitVisitorAdapter)
                            if (!disableJitVisitor.disable) {
                                fun.moreFlags |= MoreFunctionFlags.requestJit
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
        if (!empty(funcs)) {
            let totalTime = ref_time_ticks()
            // Content-address the DLL so that distinct (AST + codegen version + opt flags)
            // combinations land in distinct files. Same inputs -> same filename -> cache hit.
            // Only applied in dll-mode with the default output path; if the user pinned
            // jit_output_path explicitly or we're producing an exe, keep their path.
            var dll_hash_basename : string
            if (use_dll && config_out_path |> empty()) {
                dll_hash_basename = jit_dll_basename(funcs, opt_level, size_level, emit_prologue, debug_info)
                output_path = ".jitted_scripts/{prog.thisNamespace}/{dll_hash_basename}"
            }
            init_jit(opt_level |> uint)

            var jit_flags : LlvmJitFlags
            jit_flags.emit_prologue = emit_prologue
            jit_flags.gen_exe = gen_exe
            jit_flags.need_di = debug_info

            let attrs = new Attributes(g_ctx)
            var uids = create_uid_nodes(prog, funcs)
            var fake_visitor = new LlvmJitVisitor(ctx, g_prim_t, uids, jit_flags, null, attrs)
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
            if (recompile_prog) {
                for (fun in funcs) {
                    generate_llvm(ctx, g_prim_t, uids, attrs, fun, jit_flags)
                    let fnmna = uids.get_dll_fn_name(fun)
                    var fn_impl = LLVMGetNamedFunction(g_mod, fnmna.impl())
                    var fn = LLVMGetNamedFunction(g_mod, fnmna.publ())
                    assume maybe_mod = prog.thisModuleName |> length() == 0 ? "" : "in module {prog.thisModuleName}"
                    if (!LLVMVerifyFunction(fn_impl, LLVMVerifierFailureAction.LLVMPrintMessageAction, false)) {
                        // to_log(LOG_ERROR, "LLVM JIT: {LLVMPrintModuleToString(g_mod)}\n");
                        panic("Internal jit error. Failed to get IR of '{fun.name} implementation' {maybe_mod}.\n")
                    }
                    if (!LLVMVerifyFunction(fn, LLVMVerifierFailureAction.LLVMPrintMessageAction, false)) {
                        // to_log(LOG_ERROR, "LLVM JIT: {LLVMPrintModuleToString(g_mod)}\n");
                        panic("Internal jit error. Failed to get IR of '{fun.name}' in module {maybe_mod}.\n")
                    }
                }
                if (gen_exe) {
                    let res = inject_main(ctx, g_ctx, prog, exe_main, g_mod, g_prim_t, uids, exe_strict)
                    LINK_WHOLE_LIB = res.link_whole_lib
                    if (res.fn == null) {
                        finalize_jit(use_dll, gen_exe, g_dynamic_lib_handle)
                        // JIT failed. Couldn't create executable.
                        // Still return true, macro worked correctly.
                        return true
                    }
                }
                if (!LLVMVerifyModule(g_mod, LLVMVerifierFailureAction.LLVMPrintMessageAction, false)) {
                    g_failed = true
                }
                if (g_failed) {
                    let fn_names = join([ for (fn in funcs); "{fn.name}" ], ",")
                    panic("Internal jit error. Failed to get IR for functions {fn_names}.\n")
                }
                generate_global_ctors_dtors(g_prim_t, !(use_dll || gen_exe))
                optimize_llvm_module(opt_level |> uint(), size_level |> uint(), LLVM_INLINE_THRESHOLD)
                if (dump_ir) {
                    // Let's remove unused attributes before we print IR.
                    var pm = LLVMCreatePassManager()
                    LLVMAddGlobalDCEPass(pm)
                    LLVMRunPassManager(pm, g_mod)
                    LLVMDisposePassManager(pm)
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
                    panic("Internal jit error. Failed to get IR for functions {fn_names}.\n")
                }
                if (gen_exe) {
                    mkdir_rec(dir_name(output_path))
                    write_exe(g_mod, output_path, path_to_shared_lib, path_to_linker, LINK_WHOLE_LIB)
                } elif (use_dll) {
                    mkdir_rec(dir_name(output_path))
                    write_dll(g_mod, output_path, path_to_shared_lib, path_to_linker, LINK_WHOLE_LIB)

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
            if (use_dll) {
                assert(g_dynamic_lib_handle.handle != null)
                var extern_resolver = new ResolveExternVisitor(g_dynamic_lib_handle, g_prim_t, ctx, clone(uids))
                make_visitor(*extern_resolver) $(adapter_resolve) {
                    for (fn in funcs) {
                        visit(fn, adapter_resolve)
                    }
                }
            }
            if (!gen_exe) {
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
            var result = finalize_jit(use_dll, gen_exe, g_dynamic_lib_handle)

            set_jit_state(*ctx, result.dll, result.ee, result.ctx)

            if (LLVM_JIT_LOG) {
                let totalDt = double(get_time_usec(totalTime)) / 1000000.0lf
                to_log(LOG_INFO, "LLVM JIT: {length(funcs)} functions in {totalDt} sec\n")
            }
        }
        return true
    }
}

