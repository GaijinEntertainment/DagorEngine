options gen2
options indenting = 4
options strict_smart_pointers = false

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_dll_utils
require llvm/daslib/llvm_exe
require llvm/daslib/llvm_jit
require llvm/daslib/llvm_jit_common

require daslib/ast_boost
require daslib/strings_boost

require daslib/fio
require jit

module llvm_macro shared private


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
        get_jit_state() <| $(dll, ee, ctx : void?) {
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


[simulate_macro(name="jit_llvm")]
class JIT_LLVM : AstSimulateMacro {
    def override simulate(prog : Program?; var ctx : Context?) : bool {
        return true if (is_in_completion() || is_compiling_macros() || is_folding())
        if (!prog.policies.jit_enabled) {
            error("jit is disabled. use `-jit` option for daScript, or enable jit via CodeOfPolicies")
            return false
        }
        var funcs : array<FunctionPtr>
        var disabled : array<FunctionPtr>
        var disableJitVisitor = new DisableJitVisitor()
        var inscope disableJitVisitorAdapter <- make_visitor(*disableJitVisitor)

        assume jit_all_functions = prog.policies.jit_jit_all_functions
        assume debug_info = prog.policies.jit_debug_info
        if (prog.policies.jit_dll_mode && !das_is_dll_build()) {
            to_log(LOG_WARNING, "LLVM JIT: dll mode is enabled, however " +
                "daslang build is static, disabling dll mode.\n")
        }
        let gen_exe = prog.policies.jit_exe_mode
        let exe_main = "main" // prog.policies.jit_entry_point |> empty() ? "main" : prog.policies.jit_entry_point
        let use_dll = prog.policies.jit_dll_mode && das_is_dll_build() && !gen_exe
        let opt_level = prog.policies.jit_opt_level
        let size_level = prog.policies.jit_size_level
        let emit_prologue = prog.policies.emit_prologue
        // If you're sure `dynamic` functions is not reachable set this flag
        // to `true` and fail in runtime on `jit_trap` if they turns
        // out to be reachable.
        let exe_strict = true
        assume config_out_path = string(prog.policies.jit_output_path)
        let output_path = config_out_path |> empty() ? ".jitted_scripts/{prog.thisNamespace}" : config_out_path
        assume path_to_shared_lib = string(prog.policies.jit_path_to_shared_lib)
        assume path_to_linker = string(prog.policies.jit_path_to_linker)

        // Set global options
        LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS = prog._options |> find_arg("jit_vec3_ldu") ?as tBool ?? LLVM_JIT_ALLOW_UNALIGNED_VECTOR_READ_OUT_OF_BOUNDS_DEFAULT

        prog |> for_each_module <| $(mod) {
            mod |> for_each_function("") <| $(fun) {
                var rqj = fun.moreFlags.requestJit
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
        unsafe {
            delete disableJitVisitor
        }
        if (!empty(funcs)) {
            let totalTime = ref_time_ticks()
            init_jit(opt_level |> uint)

            var jit_flags : LlvmJitFlags
            jit_flags.emit_prologue = emit_prologue
            jit_flags.gen_exe = gen_exe
            jit_flags.need_di = debug_info

            let attrs = new Attributes(g_ctx)
            var uids = create_uid_nodes(prog, funcs)
            var fake_visitor = new LlvmJitVisitor(ctx, g_prim_t, uids, jit_flags, null, attrs)
            for (fun in funcs) {
                fake_visitor->add_llvm_functions(fun)
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
                    let fn = inject_main(ctx, g_ctx, funcs, prog, exe_main, g_mod, g_prim_t, uids, exe_strict)
                    if (fn == null) {
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
                    var fn_names = join([ for (fn in funcs); "{fn.name}" ], ",")
                    panic("Internal jit error. Failed to get IR for functions {fn_names}.\n")
                }
                generate_global_ctors_dtors(g_prim_t, !(use_dll || gen_exe))
                optimize_llvm_module(opt_level |> uint(), size_level |> uint(), LLVM_INLINE_THRESHOLD)
                if (false) {
                    // Let's remove unused attributes before we print IR.
                    var pm = LLVMCreatePassManager()
                    LLVMAddGlobalDCEPass(pm)
                    LLVMRunPassManager(pm, g_mod)
                    LLVMDisposePassManager(pm)
                    let module_str = LLVMPrintModuleToString(g_mod)
                    let msg = clone_string(module_str)
                    LLVMDisposeMessage(module_str)
                    to_log(LOG_DEBUG, "LLVM JIT: {msg}\n");
                }
                if (!LLVMVerifyModule(g_mod, LLVMVerifierFailureAction.LLVMPrintMessageAction, false)) {
                    g_failed = true
                }
                if (g_failed) {
                    var fn_names = join([ for (fn in funcs); "{fn.name}" ], ",")
                    panic("Internal jit error. Failed to get IR for functions {fn_names}.\n")
                }
                if (gen_exe) {
                    mkdir_rec(dir_name(output_path))
                    write_exe(g_mod, output_path, path_to_shared_lib, path_to_linker)
                } elif (use_dll) {
                    mkdir_rec(dir_name(output_path))
                    write_dll(g_mod, output_path, path_to_shared_lib, path_to_linker)

                    // Open just stored DLL, so we can keep working.
                    g_dynamic_lib_handle.reset()
                    g_dynamic_lib_handle = get_dll_by_path(output_path)
                    let (missing, disabling) = get_dll_missing(funcs, disabled, g_dynamic_lib_handle, uids)
                    assert(missing |> empty() && disabling |> empty())
                }
            }
            if (use_dll) {
                assert(g_dynamic_lib_handle.handle != null)
                var extern_resolver = new ResolveExternVisitor(g_dynamic_lib_handle, g_prim_t, ctx, clone(uids))
                var inscope adapter_resolve = make_visitor(*extern_resolver)
                for (fn in funcs) {
                    visit(fn, adapter_resolve)
                }
            }
            if (!gen_exe) {
                for (fun in funcs) {
                    var mangled_name = get_mangled_name(fun)
                    let MNH = hash(mangled_name)
                    var simfun = get_function_by_mangled_name_hash(MNH, *ctx)
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

