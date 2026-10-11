options gen2
options indenting = 4
options strict_smart_pointers = true

module defer shared private

//! Defer and defer_delete macros.
//!
//! ``defer`` moves a block of code to the ``finally`` section of the enclosing scope,
//! ensuring it runs on scope exit (similar to Go's ``defer``). ``defer_delete`` is deprecated
//! in favor of ``var inscope``.

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost

[tag_function(defer_tag), unused_argument(blk)]
def public defer(blk : block<() : void>) {
    //! Defers a block of code until scope exit.
    pass
}

def public nada() {
    //! helper function which does nothing and will be optimized out
    pass
}

def private defer_scope(call : Expression?; macro_name : string) : ExprBlock? {
    var scope : ExprBlock?
    compiling_program() |> get_ast_context(call) $(valid, astc) {
        if (!valid) {
            compiling_program() |> macro_error(call.at, "can't get valid program context")
        } elif (empty(astc.scopes)) {
            compiling_program() |> macro_error(call.at, "{macro_name} needs to be in the scope")
        } else {
            var blk = astc.scopes[astc.scopes |> length - 1] as ExprBlock
            if (blk.blockFlags.inTheLoop) {
                compiling_program() |> macro_error(call.at, "{macro_name} can't be used in the loop. finally section of the loop only runs once. enclose in a bare `\{ \}` block to ensure scope")
            } elif (blk.blockFlags.isLambdaBlock || blk.blockFlags.isGeneratorBlock) {
                compiling_program() |> macro_sticky_error(call.at, "{macro_name} at the top level of a lambda or generator body would become its finalizer, not a per-call cleanup. enclose in a bare `\{ \}` block, or write the finalizer as an explicit `finally`")
            } else {
                scope = blk
            }
        }
    }
    return scope
}

[tag_function_macro(tag="defer_tag")]
class DeferMacro : AstFunctionAnnotation {
    //! This macro converts defer() <| block expression
    //! into {}, and move block to the finally section of the current block
    def override transform(var call : ExprCallFunc?; var errors : das_string) : ExpressionPtr {
        var scope = defer_scope(call, "defer")
        return null if (scope == null)
        var c_arg = clone_expression(call.arguments[0])
        var from_block = move_unquote_block(c_arg)
        from_block.blockFlags ^= ExprBlockFlags.isClosure
        scope.finalList |> emplace(from_block, 0)
        return quote() <| nada()
    }
}

[call_macro(name="defer_delete")]
class DeferDeleteMacro : AstCallMacro {
    //! This macro converts defer_delete() expression
    //! into {}, and add delete expression to the finally section of the current block
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : ExpressionPtr {
        macro_verify(call.arguments |> length == 1, prog, call.at, "expecting defer_delete(expr)")
        to_compiler_log("{describe(call.at)}: *warning* defer_delete is deprecated. use var inscope instead\n")
        var scope = defer_scope(call, "defer_delete")
        return null if (scope == null)
        scope.finalList |> emplace_new <| new ExprDelete(at = call.at,
                subexpr = clone_expression(call.arguments[0])
            )
        return quote() <| nada()
    }
}
