options gen2
options no_aot

// Fixture for failed_typeless_let_variable.das: for-loop macro that rewrites
//   for (i in typeless_range(...))  into  for (i in range(...))
// but prepends a `let` whose Variable has NO type — a malformed tree the compiler
// must report (50640) instead of segfaulting in infer (the pre-guard behavior)

module _typeless_let_macro

require daslib/ast
require daslib/ast_boost

[for_loop_macro(name=typeless_let_loop)]
class TypelessLetLoop : AstForLoopMacro {
    def override visitExprFor(prog : ProgramPtr; mod : Module?; expr : ExprFor?) : ExpressionPtr {
        if (is_in_completion()) return default<ExpressionPtr>
        var idx = -1
        var src_call : ExprCall const?
        for (index, src in count(), expr.sources) {
            if (src is ExprCall) {
                let c = src as ExprCall
                if (c.name == "typeless_range") {
                    idx = index
                    src_call = c
                    break
                }
            }
        }
        if (idx == -1) return default<ExpressionPtr>
        var new_for_e = clone_expression(expr)
        var new_for = new_for_e as ExprFor
        var range_call = new ExprCall(at = src_call.at, name := "range")
        for (a in src_call.arguments) {
            range_call.arguments |> emplace_new <| clone_expression(a)
        }
        new_for.sources[idx] = range_call
        let at = expr.iteratorsAt[idx]
        var let_expr = new ExprLet(at = at)
        // the malformed bit: a Variable with no _type
        let_expr.variables |> emplace_new <| new Variable(name := "t`less", at = at,
            init = new ExprConstInt(at = at, value = 0))
        var body = new_for.body as ExprBlock
        body.list |> push(let_expr, 0)
        new_for.iteratorVariables |> clear()
        return new_for_e
    }
}
