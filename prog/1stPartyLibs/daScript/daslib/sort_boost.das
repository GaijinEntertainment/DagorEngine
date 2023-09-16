options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module sort_boost shared private

require daslib/ast_boost
require daslib/array_boost public
require daslib/macro_boost

[call_macro(name="qsort")]
class QsortMacro : AstCallMacro
    //! Implements `qsort` macro. I'ts `qsort(value,block)`.
    //! For the regular array<> or dim it's replaced with `sort(value,block)`.
    //! For the handled types like das`vector its replaced with `sort(temp_array(value),block)`.
    def override visit ( prog:ProgramPtr; mod:Module?; var expr:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        macro_verify(expr.arguments |> length==2,prog,expr.at,"expecting qsort(value, block)")
        if expr.arguments[0]._type==null || expr.arguments[1]._type==null
            return <- [[ExpressionPtr]]
        // todo: verify correct block type?
        macro_verify(expr.arguments[1]._type.baseType == Type tBlock,prog,expr.at,"expecting 2nd argument to be block(<a,b:auto(TT)>:bool))")
        if expr.arguments[0]._type.dim |> length != 0 || expr.arguments[0]._type.baseType==Type tArray
            var inscope call <- new [[ExprCall() name:="sort", at=expr.at]]
            emplace_new(call.arguments) <| clone_expression(expr.arguments[0])
            emplace_new(call.arguments) <| clone_expression(expr.arguments[1])
            return <- call
        elif expr.arguments[0]._type.baseType==Type tHandle
            var inscope call <- new [[ExprCall() name:="sort", at=expr.at]]
            var inscope tsa <- new [[ExprCall() name:="temp_array", at=expr.at]]
            emplace_new(tsa.arguments) <| clone_expression(expr.arguments[0])
            tsa.genFlags |= ExprGenFlags alwaysSafe
            emplace(call.arguments,tsa)
            emplace_new(call.arguments) <| clone_expression(expr.arguments[1])
            return <- call
        else
            macro_error(prog,expr.at,"can only qsort [], array, or handled vector")
            return <- [[ExpressionPtr]]

