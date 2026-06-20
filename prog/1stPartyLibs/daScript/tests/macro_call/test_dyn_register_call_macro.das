options gen2
options indenting = 4

require dastest/testing_boost public
require _dyn_register_decl

// Regression: a [function_macro] that dynamically registers a call_macro
// from inside its apply() must dispatch to the new instance at every
// future call site of the registered name. dasImgui's [widget] relies on
// this; if it regresses, every dasImgui boost call silently falls through
// to the underlying function without prelude/postlude/state wrapping.

[test]
def test_dyn_registered_call_macro_dispatches(t : T?) {
    t |> equal(answer(0), 42,
               "[dyn_register_42] should fold answer(N) -> 42 via the dynamically-registered call_macro")
    t |> equal(answer(123), 42,
               "argument value should be ignored (macro replaces the whole call)")
}
