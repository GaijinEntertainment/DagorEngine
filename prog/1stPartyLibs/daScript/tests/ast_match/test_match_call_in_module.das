options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_match
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ``match_call_in_module(expr, name, modName) : ExprCall?`` returns the call
// when ``expr`` is a typed ``ExprCall`` whose ``func.name`` / ``func._module.name``
// (or the ``fromGeneric`` fallback) match ``(name, modName)``.
//
// SCOPE NOTE: The positive-match path requires a fully type-inferred ``ExprCall``,
// which is reachable from ``[call_macro]`` hooks running on already-typed parent
// trees (the production usage pattern in linq_fold and sqlite_linq). It is NOT
// reliably reachable from a unit test that constructs a macro arg directly,
// because daslang resolves macro args before the macro fires but a freshly-
// constructed ``length(_arr)`` arg may not have ``.func`` populated at that
// point. The helper is byte-identical to the production version it replaces;
// the positive path is exercised transitively by PR 1b (sqlite_linq migration)
// and PR 1c (linq_fold migration).
//
// These tests pin the structural / null-guard behavior — the parts callable
// without a typed AST.

// ── Null and non-call inputs ─────────────────────────────────────────────

[test]
def test_null_does_not_match(t : T?) {
    var expr : Expression?
    t |> success(match_call_in_module(expr, "length", "builtin") == null,
                 "null input must not match")
}

[test]
def test_non_call_does_not_match(t : T?) {
    ast_gc_guard() {
        var expr = qmacro(42)
        t |> success(match_call_in_module(expr, "length", "builtin") == null,
                     "ExprConstInt must not match")
    }
}

[test]
def test_call_with_null_func_does_not_match(t : T?) {
    ast_gc_guard() {
        // Build an ExprCall by name only (no .func resolution). This is the
        // typer's pre-inference shape — match_call_in_module must return null.
        var call : Expression? = new ExprCall(at = LineInfo(), name := "length")
        t |> success(match_call_in_module(call, "length", "builtin") == null,
                     "ExprCall with null .func must not match")
    }
}
