options gen2
options indenting = 4

require dastest/testing_boost public
require _call_macro_emit_gate_helper

// Pre-declared global to anchor the right-hand side of the
// "fresh + existing" mix in test_mixed_fresh_and_existing.
var _cx_existing : int = 7

[test]
def test_two_fresh_in_one_expression(t : T?) {
    //! Two emit_or_reuse calls in ONE expression both emit fresh globals.
    //! Each macro returns ExprVar(name) for a global it just add_variable()'d.
    //! If the typer's reportAstChanged() invariant ever regresses, the
    //! second ExprVar fails to resolve in the same infer pass and this
    //! errors at compile time.
    t |> equal(emit_or_reuse_int("_cx_two_fresh_a") + emit_or_reuse_int("_cx_two_fresh_b"), 0,
               "two fresh globals emitted in same expression must both resolve, summing to 0+0")
}

[test]
def test_mixed_fresh_and_existing(t : T?) {
    //! Mix: left side is a fresh emit (_cx_mixed = 0), right side is the
    //! pre-declared _cx_existing = 7. Confirms find_variable still sees
    //! pre-existing module globals after a sibling emit in the same
    //! expression node.
    t |> equal(emit_or_reuse_int("_cx_mixed") + emit_or_reuse_int("_cx_existing"), 7,
               "fresh+existing in same expression must resolve to 0+7")
}

[test]
def test_repeated_same_name_in_one_expression(t : T?) {
    //! Same fresh name twice in one expression: first call emits, second
    //! call must find_variable() the just-emitted global and reuse it
    //! (NOT add_variable() a duplicate). Duplicate would error at typer.
    t |> equal(emit_or_reuse_int("_cx_repeat") + emit_or_reuse_int("_cx_repeat"), 0,
               "repeated name in one expression must reuse, not duplicate")
}
