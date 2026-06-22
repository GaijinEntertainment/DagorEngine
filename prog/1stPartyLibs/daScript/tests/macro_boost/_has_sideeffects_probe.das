// Helper module for tests/macro_boost/test_has_sideeffects.das.
//
// Provides ``_test_has_sideeffects(expr)`` — a [call_macro] that invokes
// ``macro_boost::has_sideeffects`` on its argument at macro time and replaces
// the call with an ``ExprConstBool`` of the result. Lets test functions
// assert side-effect classification by writing ``t |> equal(_test_has_sideeffects(...), false)``.
//
// Lives in a separate ``.das`` with a leading underscore so dastest's file
// discovery skips it as a test.
options gen2
options indenting = 4

module _has_sideeffects_probe public

require daslib/ast public
require daslib/ast_boost
require daslib/macro_boost public

[call_macro(name = "_test_has_sideeffects")]
class private TestHasSideeffects : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        if (call.arguments |> length != 1) {
            macro_error(prog, call.at, "expecting _test_has_sideeffects(expression)")
            return null
        }
        let b = has_sideeffects(call.arguments[0])
        var res : Expression? = new ExprConstBool(at = call.at, value = b)
        res.force_at(call.at)
        res.force_generated(true)
        return res
    }
}
