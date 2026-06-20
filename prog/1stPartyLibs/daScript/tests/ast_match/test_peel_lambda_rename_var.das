options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ``peel_lambda_rename_var`` peels a single-arg, single-return lambda by
// renaming its bound variable to ``argName`` in the cloned body. Non-peelable
// inputs fall through to an ``invoke($e(expr), $i(argName))`` call so callers
// can splice the result unconditionally.

// ── Helpers ─────────────────────────────────────────────────────────────

def private leftmost_var_name(var e : Expression?) : string {
    // Walks ExprOp2.left chain to the first ExprVar; returns its name or "".
    var cur = e
    while (cur != null && cur is ExprOp2) {
        cur = (cur as ExprOp2).left
    }
    if (cur is ExprVar) return string((cur as ExprVar).name)
    return ""
}

// ── Peel succeeds: renames the bound var in the body ───────────────────

[test]
def test_simple_rename(t : T?) {
    ast_gc_guard() {
        var lam = qmacro(@(x) { return x + 1; })
        var body = peel_lambda_rename_var(lam, "y")
        t |> success(body != null, "single-return lambda should peel")
        t |> success(body is ExprOp2, "peeled body should be ExprOp2")
        t |> equal(leftmost_var_name(body), "y", "bound var should be renamed to y")
    }
}

[test]
def test_rename_keeps_other_idents(t : T?) {
    ast_gc_guard() {
        // outerCapture shouldn't be touched — only x → y.
        var lam = qmacro(@(x) { return x + outerCapture; })
        var body = peel_lambda_rename_var(lam, "y")
        t |> success(body is ExprOp2, "should peel to ExprOp2")
        let op = body as ExprOp2
        t |> success(op.left is ExprVar, "left is ExprVar")
        t |> equal(string((op.left as ExprVar).name), "y", "left bound var → y")
        t |> success(op.right is ExprVar, "right is ExprVar")
        t |> equal(string((op.right as ExprVar).name), "outerCapture", "right outerCapture is preserved")
    }
}

// ── Peel fails: falls back to invoke ───────────────────────────────────

[test]
def test_two_arg_falls_back_to_invoke(t : T?) {
    ast_gc_guard() {
        var lam = qmacro(@(x, y) { return x + y; })
        var res = peel_lambda_rename_var(lam, "z")
        t |> success(res != null, "fallback should not return null")
        // ``invoke(...)`` parses to ExprInvoke, not a generic ExprCall.
        t |> success(res is ExprInvoke, "non-peelable: result should be ExprInvoke")
    }
}

[test]
def test_non_lambda_falls_back_to_invoke(t : T?) {
    ast_gc_guard() {
        var notLam = qmacro(somePtr)
        var res = peel_lambda_rename_var(notLam, "z")
        t |> success(res is ExprInvoke, "non-lambda → ExprInvoke fallback")
    }
}

// ── Clone discipline: input must not be mutated ────────────────────────

[test]
def test_input_lambda_not_mutated(t : T?) {
    ast_gc_guard() {
        var lam = qmacro(@(x) { return x + 1; })
        var _ = peel_lambda_rename_var(lam, "y")
        // Original lambda's bound-arg name must still be `x`.
        let mblk = lam as ExprMakeBlock
        let blk = mblk._block as ExprBlock
        t |> equal(string(blk.arguments[0].name), "x", "original lambda's bound var must remain `x`")
    }
}
