options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ``peel_lambda_single_return`` extracts the body of a single-arg, single-return
// lambda. The structural check is RTTI-only — no typer pass needed, so we can
// build the lambda AST inline with qmacro and assert directly.

[test]
def test_simple_return(t : T?) {
    ast_gc_guard() {
        var lam = qmacro(@(x) { return x + 1; })
        var body = peel_lambda_single_return(lam)
        t |> success(body != null, "single-return lambda should peel to its body")
        // Body should be `x + 1` — an ExprOp2.
        t |> success(body is ExprOp2, "peeled body should be ExprOp2")
    }
}

[test]
def test_two_args_does_not_peel(t : T?) {
    ast_gc_guard() {
        var lam = qmacro(@(x, y) { return x + y; })
        var body = peel_lambda_single_return(lam)
        t |> success(body == null, "2-arg lambda must return null from single-arg peel")
    }
}

[test]
def test_multi_statement_does_not_peel(t : T?) {
    ast_gc_guard() {
        var lam = qmacro(@(x) { let a = x; return a; })
        var body = peel_lambda_single_return(lam)
        t |> success(body == null, "multi-statement lambda must return null")
    }
}

[test]
def test_non_lambda_does_not_peel(t : T?) {
    ast_gc_guard() {
        var notLam = qmacro(42)
        var body = peel_lambda_single_return(notLam)
        t |> success(body == null, "non-lambda expression must return null")
    }
}

[test]
def test_null_does_not_peel(t : T?) {
    var lam : Expression?
    var body = peel_lambda_single_return(lam)
    t |> success(body == null, "null input must return null")
}
