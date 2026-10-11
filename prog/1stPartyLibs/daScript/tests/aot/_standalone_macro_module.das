options gen2

module _standalone_macro_module shared private

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost

// A [call_macro] class runs only while the requiring module compiles. Standalone
// AOT must not carry it into the simulated context - the point of the macro is
// that its work is finished before the runtime exists.
[call_macro(name="bake_sum")]
class BakeSum : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(expr.arguments |> length == 1, prog, expr.at, "bake_sum expects one integer literal")
        macro_verify(expr.arguments[0] is ExprConstInt, prog, expr.at,
            "bake_sum: argument must be an integer literal, got {describe(expr.arguments[0])}")
        let n = (expr.arguments[0] as ExprConstInt).value
        var total = 0
        for (i in range(n + 1)) {
            total += i
        }
        return <- qmacro($v(total))
    }
}
