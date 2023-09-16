module macros

require rtti
require ast public
require daslib/ast_boost public

def private prepend_cond(var fn:FunctionPtr; cond:string)
    var iblk <- fn.body
    var blk <- new [[ExprBlock() at=fn.at]]
    var ifm <- new [[ExprCall() at=fn.at, name:=cond]]
    var ife <- new [[ExprIfThenElse() at=fn.at, cond<-ExpressionPtr(ifm), if_true:=ExpressionPtr(iblk)]]
    push(blk.list,ExpressionPtr(ife))
    unsafe
        fn.body <- ExpressionPtr(blk)


class AppendCondAnnotation : AstFunctionAnnotation
    def abstract getCondName() : string

    def override apply ( var func:FunctionPtr; var group:ModuleGroup; args:AnnotationArgumentList; var errors : das_string ) : bool
        prepend_cond(func, self->getCondName())
        return true