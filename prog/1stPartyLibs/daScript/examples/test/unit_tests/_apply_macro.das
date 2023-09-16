module _apply_macro

require ast
require daslib/ast_boost

def private prepend_cond(var fn:FunctionPtr; cond:string)
    var iblk <- fn.body
    var blk <- new [[ExprBlock() at=fn.at]]
    var ifm <- new [[ExprCall() at=fn.at, name:=cond]]
    var ife <- new [[ExprIfThenElse() at=fn.at, cond<-ifm, if_true<-iblk]]
    emplace(blk.list,ife)
    fn.body <- blk

[function_macro(name="add_cond")]
class AppendCondAnnotation : AstFunctionAnnotation
    def override apply ( var func:FunctionPtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        var cond = ""
        let argName =args |> find_arg("cond")
        if !(argName is tString)
            panic("cond must be string")
        else
            prepend_cond(func,argName as tString)
        return true
