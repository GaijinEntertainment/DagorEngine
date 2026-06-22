options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ``peel_lambda_rename_2vars`` is the 2-arg variant (shape: aggregate's
// ``block<(acc, x) : AGG>``). Unlike the 1-arg form, this returns ``null``
// for non-peelable inputs — caller decides the fallback.

[test]
def test_simple_rename(t : T?) {
    ast_gc_guard() {
        var lam = qmacro(@(acc, x) { return acc + x; })
        var body = peel_lambda_rename_2vars(lam, "a", "b")
        t |> success(body != null, "2-arg lambda should peel")
        t |> success(body is ExprOp2, "result should be ExprOp2")
        let op = body as ExprOp2
        t |> success(op.left is ExprVar, "left should be renamed ExprVar")
        t |> equal(string((op.left as ExprVar).name), "a", "acc → a")
        t |> success(op.right is ExprVar, "right should be renamed ExprVar")
        t |> equal(string((op.right as ExprVar).name), "b", "x → b")
    }
}

[test]
def test_one_arg_returns_null(t : T?) {
    ast_gc_guard() {
        var lam = qmacro(@(x) { return x + 1; })
        var body = peel_lambda_rename_2vars(lam, "a", "b")
        t |> success(body == null, "1-arg lambda must return null (no fallback)")
    }
}

[test]
def test_three_arg_returns_null(t : T?) {
    ast_gc_guard() {
        var lam = qmacro(@(x, y, z) { return x + y + z; })
        var body = peel_lambda_rename_2vars(lam, "a", "b")
        t |> success(body == null, "3-arg lambda must return null")
    }
}

[test]
def test_multi_statement_returns_null(t : T?) {
    ast_gc_guard() {
        var lam = qmacro(@(acc, x) { let t = acc; return t + x; })
        var body = peel_lambda_rename_2vars(lam, "a", "b")
        t |> success(body == null, "multi-statement body must return null")
    }
}

[test]
def test_non_lambda_returns_null(t : T?) {
    ast_gc_guard() {
        var notLam = qmacro(42)
        var body = peel_lambda_rename_2vars(notLam, "a", "b")
        t |> success(body == null, "non-lambda must return null")
    }
}
