options gen2
options indenting = 4

module llvm_code_shell shared private

require daslib/ast
require daslib/ast_boost
require llvm/daslib/llvm_code

//! The `[llvm_code]` spelling for dasLLVM's own daslib (`llvm_tune`'s fat-word stubs): `daslib/tune`
//! carries the user-facing annotation and requires `llvm_tune`, so `llvm_tune` cannot see it. This
//! one is visible only where the module is required directly - never publicly, and never from a
//! program, which would then see two annotations of that name.

[function_macro(name = "llvm_code")]
class private LlvmCodeShellLocal : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup;
                       var args : AnnotationArgumentList; var errors : das_string) : bool {
        return llvm_code_apply(func, group, args, errors)
    }
}
