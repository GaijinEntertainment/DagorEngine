options indenting = 4
options no_aot = true

module extra_ann shared

require ast
require rtti
require daslib/ast_boost

[function_macro(name="test_extra")]
class TestExtraAnnotation : AstFunctionAnnotation
    def override apply ( var func:FunctionPtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        append_annotation(func,"$","marker",[{auto
            [[auto "foo", [[RttiValue tBool=true]] ]];
            [[auto "bar", [[RttiValue tInt=13]] ]];
            [[auto "pi", [[RttiValue tFloat=3.14]] ]];
            [[auto "test", [[RttiValue tString="hello,world"]] ]]
        }])
        return true
