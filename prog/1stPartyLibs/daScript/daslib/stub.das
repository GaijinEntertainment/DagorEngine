options gen2
options indenting = 4
options strict_smart_pointers = true

module stub shared private

require daslib/ast_boost
require daslib/templates_boost

[function_macro(name="stub")]
class TemplateMacro : AstFunctionAnnotation {
    //! Function annotation stub for stubbed-out modules.
    def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        if (func.result == null) {
            errors := "stub function must have a result type"
            return false
        }
        func.sideEffectFlags.userScenario = true
        for (arg in func.arguments) {
            arg.flags.marked_used = true
        }
        if (func.result.baseType == Type.tVoid) return true
        var resType = clone_type(func.result)
        if (!resType.flags.ref) {
            resType.flags.constant = false
        }
        var qblk = qmacro_block() {
            panic("stub function called")
            unsafe {
                var dummy : $t(resType)
                return <- dummy
            }
        }
        qblk |> force_at(func.body.at)
        // The body is macro-generated, not user-written — mark it so lint passes
        // (e.g. STYLE025 on the generated `unsafe { var dummy ... }`) skip it.
        qblk |> force_generated(true)
        func.body = qblk
        return true
    }
}

