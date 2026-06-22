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
require llvm/daslib/llvm_jit_cli
require llvm/daslib/llvm_jit_run

require daslib/ast_boost
require daslib/strings_boost
require daslib/defer

require daslib/fio
require daslib/clargs
require jit

module llvm_macro shared private

// Lives here (not in llvm_jit_run) so the standalone-exe codegen's skip list
// in llvm_exe.das — which excludes llvm_func/llvm_macro from the must-be-JIT-able
// function set — keeps this [no_jit] finalizer out of exe builds.
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

[simulate_macro(name="jit_llvm")]
class JIT_LLVM : AstSimulateMacro {
    def override simulate(prog : Program?; var ctx : Context?) : bool {
        return true if (is_in_completion() || is_compiling_macros() || is_folding())
        return run_jit(prog, ctx)
    }
}
