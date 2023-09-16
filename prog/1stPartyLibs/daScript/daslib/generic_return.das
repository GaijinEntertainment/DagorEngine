options no_unused_block_arguments = false
options no_unused_function_arguments = false
options indenting = 4
options no_aot = true
options strict_smart_pointers = true

module generic_return shared private

require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost

[call_macro(name="generic_return")]
class GenericReturn : AstCallMacro
    //! This macro convert fake_numeric(func_id) to the comprehensive faker test::
    def override visit ( prog:ProgramPtr; mod:Module?; var expr:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        if expr.arguments |> length !=1
            macro_error(prog,expr.at,"generic_return expecting 1 argument")
            return <- [[ExpressionPtr]]
        assume ret_expr = expr.arguments[0]
        if ret_expr._type==null || ret_expr._type.isAutoOrAlias
            macro_error(prog,expr.at,"generic_return requires fully inferred type")
            return <- [[ExpressionPtr]]
        if ret_expr._type.isVoid
            return <- qmacro_block <|
                $e(ret_expr)
                return
        elif ret_expr._type.canCopy
            return <- qmacro_block <|
                return $e(ret_expr)
        elif ret_expr._type.canMove
            return <- qmacro_block <|
                return <- $e(ret_expr)
        else
            macro_error(prog,expr.at,"generic_return for the type {describe(ret_expr._type)}, which can't be copied or moved")
            return <- [[ExpressionPtr]]

