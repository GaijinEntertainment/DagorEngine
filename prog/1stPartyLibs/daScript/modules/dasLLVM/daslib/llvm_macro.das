options gen2
options indenting = 4
options strict_smart_pointers = false
options stack = 4_194_304
options no_global_variables = false

module llvm_macro shared private

//! The emitter's macro context. Nothing requires this module from a program: `llvm_jit_link`
//! brings it in through `require_module_now` - on a DLL cache miss, and for every artifact
//! but a cached DLL - and calls the two entries below by name in this context.

require llvm/daslib/llvm_boost
require llvm/daslib/llvm_jit_run
require daslib/ast_boost

[_macro]
def private llvm_macro_context {
    pass
}

//! the whole codegen pipeline for a simulated program; `ok` is `run_jit`'s answer
[export]
def public run_jit_codegen(prog : Program?; var ctx : Context?; var ok : bool?) {
    *ok = run_jit(prog, ctx).ok
}

[finalize(late)]
def private shutdown_llvm {
    LLVMShutdown()
}

//! the in-memory engine's teardown; the finalizer in `llvm_jit_link` delegates here
[export]
def public free_jit_engine(ee : void?; ctx : void?) {
    assert(ee != null && ctx != null)
    ee |> LLVMRunStaticDestructors()
    LLVMDisposeExecutionEngine(unsafe(reinterpret<LLVMExecutionEngineRef>(ee)))
    LLVMContextDispose(unsafe(reinterpret<LLVMContextRef>(ctx)))
}
