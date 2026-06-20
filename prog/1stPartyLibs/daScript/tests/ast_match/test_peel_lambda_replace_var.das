options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ``peel_lambda_replace_var`` peels a single-arg, single-return lambda by
// substituting its bound variable with an arbitrary expression in the cloned
// body (peeling-aware: strips typer ``ExprRef2Value`` wrappers from already-
// typed source ASTs). Non-peelable inputs fall back to ``invoke($e(expr), $e(replacement))``.

// ── Substitution-success path requires typed AST (ExprRef2Value wrappers) ──
// ``replaceVariablePeeling`` only fires on ``ExprRef2Value(ExprVar(name))``, not
// on a bare ``ExprVar``. The typer inserts these wrappers around reference-typed
// reads at inference time — raw ``qmacro``-built AST never has them. Build the
// wrapped shape manually to exercise the success path.
//
// apply_template mutates AST children in place but cannot replace the cloned
// root pointer (matches production usage in linq_fold: the var-being-replaced
// is always buried inside a larger expression like ``x + 1``, never at root).
// So the test body uses ``x + 1`` rather than bare ``x``.

def private wrap_ref2value_around_lhs_var(var lam : Expression?) : Expression? {
    // Lambda is ``@(x) { return x + 1; }``. Wrap the ExprVar ``x`` (the .left of
    // ret.subexpr's ExprOp2) in an ExprRef2Value, mimicking what the typer does.
    var mblk = lam as ExprMakeBlock
    var blk = mblk._block as ExprBlock
    var ret = blk.list[0] as ExprReturn
    var op = ret.subexpr as ExprOp2
    op.left = new ExprRef2Value(at = op.left.at, subexpr = op.left)
    return lam
}

[test]
def test_replaces_buried_var(t : T?) {
    ast_gc_guard() {
        var raw = qmacro(@(x) { return x + 1; })
        var lam = wrap_ref2value_around_lhs_var(raw)
        var replacement : Expression? = qmacro(myVal)
        var body = peel_lambda_replace_var(lam, replacement)
        t |> success(body != null, "should peel")
        t |> success(body is ExprOp2, "result should be ExprOp2")
        let op = body as ExprOp2
        t |> success(op.left is ExprVar, "left should be substituted ExprVar (R2V wrapper stripped)")
        t |> equal(string((op.left as ExprVar).name), "myVal", "bound var was substituted with the replacement var")
    }
}

[test]
def test_two_arg_falls_back_to_invoke(t : T?) {
    ast_gc_guard() {
        var lam = qmacro(@(x, y) { return x + y; })
        var replacement : Expression? = qmacro(z)
        var res = peel_lambda_replace_var(lam, replacement)
        // ``invoke(...)`` parses to ExprInvoke, not a generic ExprCall.
        t |> success(res is ExprInvoke, "non-peelable → ExprInvoke fallback")
    }
}

[test]
def test_non_lambda_falls_back_to_invoke(t : T?) {
    ast_gc_guard() {
        var notLam = qmacro(somePtr)
        var replacement : Expression? = qmacro(z)
        var res = peel_lambda_replace_var(notLam, replacement)
        t |> success(res is ExprInvoke, "non-lambda → ExprInvoke fallback")
    }
}
