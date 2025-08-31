options gen2
options indenting = 4
options no_aot = true

module extra_ann shared

require ast
require rtti
require daslib/ast_boost

[function_macro(name="test_extra")]
class TestExtraAnnotation : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        let foo =  ("foo", RttiValue(tBool = true))
        let bar =  ("bar", RttiValue(tInt = 13))
        let pi =  ("pi", RttiValue(tFloat = 3.14))
        let test =  ("test", RttiValue(tString = "hello,world"))
        append_annotation(func, "$", "marker", [
            foo, bar, pi, test
        ])
        return true
    }
}
