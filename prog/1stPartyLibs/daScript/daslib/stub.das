options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module stub shared private

require daslib/ast_boost
require daslib/templates_boost

[function_macro(name="stub")]
class TemplateMacro : AstFunctionAnnotation
    def override apply ( var func:FunctionPtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        if func.result==null
            errors := "stub function must have a result type"
            return false
        func.sideEffectFlags |= FunctionSideEffectFlags userScenario
        for arg in func.arguments
            arg.flags |= VariableFlags marked_used
        if func.result.baseType == Type tVoid
            return true
        var inscope resType <- clone_type(func.result)
        if !resType.flags.ref
            resType.flags &= ~TypeDeclFlags constant
        var inscope qblk <- qmacro_block <|
            panic("stub function called")
            unsafe
                var dummy : $t(resType)
                return <- dummy
        qblk |> force_at(func.body.at)
        move(func.body) <| qblk
        return true

