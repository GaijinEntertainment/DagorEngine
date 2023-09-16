options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options no_aot = true
options strict_smart_pointers = true

module defer shared private

require ast
require rtti
require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost

[tag_function(defer_tag)]
def public defer ( blk : block<():void> )
    //! defer a block of code. For example::
    //!
    //!     var a = fopen("filename.txt","r")
    //!     defer <|
    //!         fclose(a)
    //!
    //! Will close the file when 'a' is out of scope.
    pass

def public nada()
    //! helper function which does nothing and will be optimized out
    pass

[tag_function_macro(tag="defer_tag")]
class DeferMacro : AstFunctionAnnotation
    //! This macro covnerts defer() <| block expression
    //! into {}, and move block to the finally section of the current block
    def override transform ( var call : smart_ptr<ExprCallFunc>; var errors : das_string ) : ExpressionPtr
        var success = true
        compiling_program() |> get_ast_context(call) <| $(valid, astc)
            if !valid
                compiling_program() |>macro_error(call.at,"can't get valid program context")
                success = false
            elif astc.scopes |> length == 0
                compiling_program() |>macro_error(call.at,"defer needs to be in the scope")
                success = false
            else
                var scope = astc.scopes[astc.scopes |> length-1] as ExprBlock
                if scope.blockFlags.inTheLoop
                    compiling_program() |>macro_error(call.at,"defer can't be used in the loop. finally section of the loop only runs one. enclose in `if true` to ensure scope")
                    success = false
                else
                    var inscope c_arg <- clone_expression(call.arguments[0])
                    var inscope from_block <- move_unquote_block(c_arg)
                    from_block.blockFlags ^= ExprBlockFlags isClosure
                    scope.finalList |> emplace(from_block,0)
        return <- !success ? [[ExpressionPtr]] : quote() <| nada()

[call_macro(name="defer_delete")]
class DeferDeleteMacro : AstCallMacro
    //! This macro converts defer_delete() expression
    //! into {}, and add delete expression to the finally section of the current block
    def override visit ( prog:ProgramPtr; mod:Module?; var call:smart_ptr<ExprCallMacro> ) : ExpressionPtr
        macro_verify(call.arguments |> length==1,prog,call.at,"expecting defer_delete(expr)")
        to_compiler_log("{describe(call.at)}: *warning* defer_delete is deprecated. use var inscope instead\n")
        var success = true
        prog |> get_ast_context(call) <| $(valid, astc)
            if !valid
                compiling_program() |>macro_error(call.at,"can't get valid program context")
                success = false
            elif astc.scopes |> length == 0
                compiling_program() |>macro_error(call.at,"defer_delete needs to be in the scope")
                success = false
            else
                var scope = astc.scopes[astc.scopes |> length-1] as ExprBlock
                if scope.blockFlags.inTheLoop
                    compiling_program() |>macro_error(call.at,"defer_delete can't be used in the loop. finally section of the loop only runs one. enclose in `if true` to ensure scope")
                    success = false
                else
                    scope.finalList |> emplace_new <| new [[ExprDelete() at=call.at,
                            subexpr <- clone_expression(call.arguments[0])
                        ]]
        return <- !success ? [[ExpressionPtr]] : quote() <| nada()
