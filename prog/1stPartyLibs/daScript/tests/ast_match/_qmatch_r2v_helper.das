// Helper module for tests/ast_match/test_ref2value_skip.das.
//
// Provides ``test_emit_qmatch_with_wrapped_pattern(source)`` — a [call_macro]
// that hand-builds an ``ExprCallMacro(name="qmatch")`` whose pattern argument
// is an ``ExprRef2Value(...)`` wrapper around a clean inner pattern. This
// exercises the pattern-side peel in ``generate_match``, which is reachable
// in production through macro composition (a meta-macro can construct a
// ``qmatch(...)`` call and feed it a pattern AST that already carries
// typer-inserted ``ExprRef2Value`` wrappers from a value-context read).
//
// Lives in a separate ``.das`` with a leading underscore so dastest's file
// discovery skips it as a test (per ``feedback_dastest_underscore_skip``).
options gen2
options indenting = 4

module _qmatch_r2v_helper public

require daslib/ast public
require daslib/ast_boost
require daslib/templates_boost
require daslib/ast_match public

[call_macro(name = "test_emit_qmatch_with_wrapped_pattern")]
class private EmitQMatchWithWrappedPattern : AstCallMacro {
    def override canVisitArgument(call : ExprCallMacro?; argIndex : int) : bool {
        return false
    }
    def override visit(prog : ProgramPtr; mod : Module?; var call : ExprCallMacro?) : Expression? {
        if (call.arguments |> length != 1) {
            macro_error(prog, call.at, "expecting test_emit_qmatch_with_wrapped_pattern(source)")
            return null
        }
        let at = call.at

        // Inner pattern — clean ExprField shape (a.foo).
        var inner_pattern = qmacro(a.foo)

        // Wrap the pattern in ExprRef2Value, exactly the shape the typer
        // would insert if a meta-macro fed a typed-as-value expression
        // into qmatch's pattern slot.
        var wrapped_pattern : Expression? = new ExprRef2Value(at = at, subexpr = inner_pattern)

        // Build qmatch via qmacro (which sets ExprCallMacro.macro correctly
        // through the normal parser path), then patch arguments[1] with the
        // wrapped pattern. Naive `qmacro(qmatch($e(src), $e(wrapped)))` would
        // fail because qmatch.canVisitArgument(1)=false keeps the pattern
        // arg raw — qmacro's `$e` tag survives into qmatch's pattern slot
        // where qmatch reads it as its own capture syntax. Using a literal
        // `0` placeholder for the pattern slot avoids the collision; we
        // overwrite it post-build.
        var src_arg = call.arguments[0]
        var qm = qmacro(qmatch($e(src_arg), 0))
        (qm as ExprCallMacro).arguments[1] = wrapped_pattern
        return qm
    }
}
