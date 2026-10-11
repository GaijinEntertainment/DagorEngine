// Call macros whose quoted literal must survive the lowered quote reconstruction
// (options aot_macros) with its literal mark, so the splice still adapts to a uint / int64.
options gen2
options indenting = 4
options aot_macros

module _quote_lit_macro public

require daslib/ast public
require daslib/ast_boost
require daslib/templates_boost

[call_macro(name = "plus_one_lit")]
class private PlusOneLit : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        if (call.arguments |> length != 1) {
            macro_error(prog, call.at, "expecting plus_one_lit(value)")
            return null
        }
        var arg = call.arguments[0]
        return qmacro($e(arg) + 1)
    }
}

[call_macro(name = "plus_spliced_lit")]
class private PlusSplicedLit : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        if (call.arguments |> length != 1) {
            macro_error(prog, call.at, "expecting plus_spliced_lit(value)")
            return null
        }
        var arg = call.arguments[0]
        let n = 2
        return qmacro($e(arg) + $v(n))
    }
}
