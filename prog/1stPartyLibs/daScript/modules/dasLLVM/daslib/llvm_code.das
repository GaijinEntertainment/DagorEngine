options gen2
options indenting = 4
options no_aot = true

module llvm_code shared public

require daslib/ast
require daslib/rtti
require daslib/ast_boost

//! `[llvm_code(name="generator_key", ...)]` marks a function for external JIT codegen: when the
//! LLVM JIT compiles it, the das generator registered under `generator_key` (see
//! `llvm/daslib/llvm_jit_code.das`, wired in via `llvm/daslib/llvm_user_modules.das`) emits its
//! IR instead of the body. The body stays the reference implementation — the interpreter, AOT,
//! and any target the generator declines compile it verbatim. Extra annotation arguments are
//! the generator's parameters (and fold into the JIT DLL cache key).
//! The annotation itself is registered by `daslib/tune` (inert without dasLLVM); this is the
//! body its shell forwards to.
def llvm_code_apply(var func : FunctionPtr; var _group : ModuleGroup;
                    args : AnnotationArgumentList; var errors : das_string) : bool {
    for (arg in args) {
        if (arg.name == "name") {
            if (arg.basicType != Type.tString || empty(arg.sValue)) {
                errors := "[llvm_code] on {func.name}: `name` must be a non-empty string (the generator key)"
                return false
            }
            // force side effects, or constant-argument calls const-fold in the front-end and never reach the generator
            func.sideEffectFlags.userScenario = true
            return true
        }
    }
    errors := "[llvm_code] on {func.name}: missing name=\"generator_key\" argument"
    return false
}
