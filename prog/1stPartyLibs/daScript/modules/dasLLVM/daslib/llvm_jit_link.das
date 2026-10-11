options gen2
options indenting = 4
options strict_smart_pointers = false
options stack = 4_194_304

module llvm_jit_link shared private

//! The JIT's entry, and the whole of a DLL cache hit. `just_in_time` requires this module and
//! nothing of the emitter: the `jit_llvm` simulate macro computes the plan, opens the
//! content-addressed DLL and binds the functions it holds. A miss, and every artifact but a
//! cached DLL - an exe, a wasm, an AOT object, compile-only, an in-memory run, a pinned output
//! path - brings the emitter's macro module in through `require_module_now` and runs `run_jit`
//! there, so a warm run never loads the emitter's contexts.

require llvm/daslib/llvm_jit_plan
require daslib/ast_boost
require daslib/cross_context
require jit

let private EMITTER_MODULE = "llvm/daslib/llvm_macro"

[simulate_macro(name="jit_llvm")]
class JIT_LLVM : AstSimulateMacro {
    def override simulate(prog : Program?; var ctx : Context?) : bool {
        return true if (is_in_completion() || is_compiling_macros() || is_folding())
        return run_jit_linked(prog, ctx)
    }
}

//! the link side's plan, its probe and the emitter's load are JIT work the emitter's own report
//! never covers, so the miss path prints them on one line
def private run_jit_in_emitter(prog : Program?; var ctx : Context?; log_jit_time : bool; t_plan, t_probe : int) : bool {
    let load_tm = ref_time_ticks()
    let mctx = macro_context_of(EMITTER_MODULE)
    if (mctx == null) {
        panic("LLVM JIT: the emitter module {EMITTER_MODULE} did not load")
    }
    if (log_jit_time) {
        to_log(LOG_INFO, "LLVM JIT time: plan {jit_sec(t_plan)} probe {jit_sec(t_probe)} emitter load {jit_sec(get_time_usec(load_tm))}\n")
    }
    var ok = false
    unsafe {
        invoke_in_context(mctx, "run_jit_codegen", prog, ctx, addr(ok))
    }
    return ok
}

def private drop_probe(var dll : DLLHandle?; var uids : UidNodes?) {
    dll.reset()
    unsafe {
        delete dll
        delete uids
    }
}

//! the hit path: plan, probe the DLL the plan names, bind what it holds; anything else is the
//! emitter's. Returns true - a miss is codegen, not a failure.
[macro_function]
def public run_jit_linked(prog : Program?; var ctx : Context?) : bool {
    if (prog.policies.aot && !prog.policies.jit_emit_object && !prog.policies.jit_enabled) {
        return true
    }
    let totalTime = ref_time_ticks()
    var phase_tm = ref_time_ticks()
    var plan <- make_jit_plan(prog, ctx, true)
    return false if (report_collect_errors(plan))
    let t_hash = get_time_usec(phase_tm)
    if (!plan.use_dll) {
        return run_jit_in_emitter(prog, ctx, plan.log_jit_time, t_hash, 0)
    }
    if (empty(plan.candidates) && !plan.gen_lib) {
        to_log(LOG_INFO, "LLVM JIT: 0 functions to jit ({length(plan.disabled)} marked no_jit) - the WHOLE program runs interpreted\n")
        return true
    }
    phase_tm = ref_time_ticks()
    var uids = create_uid_nodes(prog, plan.candidates, false)
    var dll = get_dll_by_path(plan.output_path)
    var probe <- probe_dll(plan.candidates, plan.disabled, dll, uids)
    let t_probe = get_time_usec(phase_tm)
    // a DLL holding none of the candidates is not a cache of this program: a pinned
    // `-output` path another program wrote
    if (dll.handle == null || probe.mismatched != 0 || empty(probe.bound)) {
        drop_probe(dll, uids)
        return run_jit_in_emitter(prog, ctx, plan.log_jit_time, t_hash, t_probe)
    }
    phase_tm = ref_time_ticks()
    mark_jit_selection(prog, probe.bound)
    let errors <- resolve_dll_externs(prog, dll, ctx, uids, probe.bound)
    if (!empty(errors)) {
        // a process fact the DLL name does not fold - a builtin with no interop address here, a
        // handled type's helper from a module this process lacks: the emitter's content gate
        // decides again and rewrites the DLL, where a panic would repeat on every run
        for (e in errors) {
            to_log(LOG_WARNING, "LLVM JIT: {e}\n")
        }
        to_log(LOG_WARNING, "LLVM JIT: the cached {plan.output_path}.dll cannot be linked into this process - {length(errors)} extern(s) unresolved, codegen\n")
        drop_probe(dll, uids)
        return run_jit_in_emitter(prog, ctx, plan.log_jit_time, t_hash, t_probe)
    }
    to_log(LOG_INFO, "LLVM JIT: DLL cache hit {plan.output_path}.dll\n")
    install_dll_functions(dll, ctx, uids, probe.bound)
    let t_install = get_time_usec(phase_tm)
    phase_tm = ref_time_ticks()
    set_jit_state(*ctx, dll.handle, null, null)
    unsafe {
        delete uids
    }
    let t_finalize = get_time_usec(phase_tm)
    jit_log_report(probe.bound, false, plan.opt_level, !empty(plan.dll_hash_basename), plan.jit_split, 0, plan.debug_info, plan.log_jit_time, totalTime,
        t_hash, 0, 0, t_probe, 0, 0, 0, t_install, t_finalize)
    return true
}

//! Frees the jit state when the last context sharing it goes: the cached DLL closes here; an
//! in-memory engine is the emitter's to dispose, and the emitter is in the process whenever
//! there is one. One function, so a jitted program carries one finalizer and nothing else.
[finalize(late), no_jit]
def public free_jit_context {
    if (get_context_share_counter() == 1ul) {
        get_jit_state() $(dll, ee, ctx : void?) {
            if (dll != null) {
                close_dynamic_library(dll)
            } elif (ee != null || ctx != null) {
                let emitter = get_module("llvm_macro")
                if (emitter == null || !has_macro_context(emitter)) {
                    panic("LLVM JIT: an in-memory engine exists but the emitter module is not in the process")
                }
                unsafe(invoke_in_context(find_macro_context(emitter), "free_jit_engine", ee, ctx))
            }
        }
    }
}
