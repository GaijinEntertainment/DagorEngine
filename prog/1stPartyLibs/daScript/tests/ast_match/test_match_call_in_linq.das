options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ``match_call_in_linq(expr, name) : ExprCall?`` is a thin wrapper on
// ``match_call_in_module(expr, name, "linq")``.
//
// SCOPE NOTE: see test_match_call_in_module.das — positive matches require
// typed AST and are covered transitively by PR 1b/1c. These tests pin the
// structural / null-guard behavior callable without a typed AST.

[test]
def test_null_does_not_match(t : T?) {
    var expr : Expression?
    t |> success(match_call_in_linq(expr, "_select") == null,
                 "null input must not match")
}

[test]
def test_non_call_does_not_match(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(42)
        t |> success(match_call_in_linq(expr, "_select") == null,
                     "non-call must not match")
    }
}

[test]
def test_call_with_null_func_does_not_match(t : T?) {
    ast_gc_guard() {
        var call : Expression? = new ExprCall(at = LineInfo(), name := "_select")
        t |> success(match_call_in_linq(call, "_select") == null,
                     "ExprCall with null .func must not match")
    }
}
