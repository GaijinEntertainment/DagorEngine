options gen2
options indenting = 4

require dastest/testing_boost public
require _call_macro_emit_gate_helper

// Pre-declared global. test_reuse_existing_global verifies that
// emit_or_reuse_int("_gate_b") detects this via find_variable and
// hands back the SAME global (read = 999), rather than emitting
// a duplicate that shadows it.
var _gate_b : int = 999

[test]
def test_emit_when_absent(t : T?) {
    //! Fresh name (_gate_a not declared anywhere): macro emits a new
    //! int global with the default zero-init, ExprVar reads back 0.
    t |> equal(emit_or_reuse_int("_gate_a"), 0,
               "fresh name should be emitted as a new int global with default 0")
}

[test]
def test_reuse_existing_global(t : T?) {
    //! Pre-declared name (_gate_b = 999 above): macro detects the
    //! existing global via find_variable and re-uses it. If gating
    //! regresses and add_variable is called anyway, the typer sees
    //! a duplicate and either errors or shadows — either way this
    //! test fails loud.
    t |> equal(emit_or_reuse_int("_gate_b"), 999,
               "existing global must be reused, not overwritten or duplicated")
}
