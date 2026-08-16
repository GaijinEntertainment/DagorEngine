// helper module for failed_macro_error_in_patch
//
// The [patch_error_after_mutate] function_macro raises a deliberate macro_error during patch()
// on the SAME pass it mutates the AST (astChanged=true, one-shot via a sentinel global). This is
// the exact shape that used to silently drop the error: the astChanged re-infer cleared
// program->errors at the top of the next infer pass before the failure could surface, so a
// fail-closed codegen guard (e.g. a Vulkan shader's >1-UBO bind guard) shipped a no-op instead of
// a compile error. The compiler must now keep the error and fail the compile.
options gen2

module _helper_macro_patch_error public

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost

[function_macro(name = "patch_error_after_mutate")]
class PatchErrorAfterMutate : AstFunctionAnnotation {
    def override patch(var func : FunctionPtr; var group : ModuleGroup;
                       args, progArgs : AnnotationArgumentList; var errors : das_string;
                       var astChanged : bool&) : bool {
        let sentinel = "_pe_sentinel_{func.name}"
        var glob <- find_variable(compiling_module(), sentinel)
        if (glob == null) {
            // mutate the AST so this pass reports astChanged=true (forcing the re-infer that
            // used to wipe program->errors), then raise the deliberate error on the same pass
            var t = new TypeDecl(at = func.at, baseType = Type.tInt)
            add_global_var(compiling_module(), sentinel, t, func.at, false)
            astChanged = true
            macro_error(compiling_program(), func.at,
                "deliberate patch-time macro_error after an AST mutation")
        }
        glob := null
        return true
    }
}
