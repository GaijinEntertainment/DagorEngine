// helper module for macro_patch_per_instance
//
// [count_patch] is a function_macro whose patch() splices one `double_patch_marks++`
// into the annotated body and marks ITS OWN argument list `patched`. Two [count_patch]
// annotations on one function must each patch once: the guard reads and writes the
// `args` the compiler handed to this call, never a by-name search over func.annotations
// (which cannot tell two same-named instances apart and used to mark both on the first).
options gen2

module _helper_double_patch public

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost

var double_patch_marks = 0

[function_macro(name = "count_patch")]
class CountPatch : AstFunctionAnnotation {
    def override patch(var func : FunctionPtr; var group : ModuleGroup;
                       var args : AnnotationArgumentList; progArgs : AnnotationArgumentList;
                       var errors : das_string; var astChanged : bool&) : bool {
        if (find_arg(args, "patched") is tBool) return true
        var body = func.body as ExprBlock
        body.list |> emplace_new <| qmacro_expr() {
            double_patch_marks++
        }
        args |> add_annotation_argument("patched", true)
        astChanged = true
        return true
    }
}
