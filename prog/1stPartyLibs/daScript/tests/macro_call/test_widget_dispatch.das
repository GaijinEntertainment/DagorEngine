options gen2
options indenting = 4

require dastest/testing_boost public
require _widget_dispatch_helper

[widget]
def my_button(var state : StubState; text : string) : int {
    return state.value
}

[test]
def test_widget_call_macro_fires(t : T?) {
    //! `my_button(BTN_A, "click me")` should be intercepted by the [widget]
    //! call_macro: it auto-emits BTN_A as a module global of type StubState,
    //! then rewrites the call to `my_button(BTN_A, "BTN_A", "click me")`,
    //! matching the macro-expanded function signature
    //! `def my_button(state, widget_ident, text)`.
    //!
    //! If the call_macro never fires, the typer sees a 2-arg call against
    //! a 3-arg function and errors with "no matching functions or generics".
    //!
    //! If the auto-emit never happens, BTN_A is unresolved and the typer
    //! errors with "can't locate variable 'BTN_A'".
    let v = my_button(BTN_A, (text = "click me"))
    t |> equal(v, 0, "auto-emitted BTN_A's StubState defaults to value=0")
}

[test]
def test_widget_global_visible_outside_call(t : T?) {
    //! After `my_button(BTN_B, ...)` auto-emits BTN_B, a separate
    //! ExprVar(BTN_B) elsewhere in the module must resolve to that global.
    //! This is the cross-expression resolution path (gap #3 in the
    //! dasImgui plan) — guaranteed by reportAstChanged() in
    //! ast_infer_type.cpp:5220.
    let v = my_button(BTN_B, (text = "x"))
    t |> equal(v, 0, "fresh BTN_B reads as value=0")
    BTN_B.value = 42
    let v2 = my_button(BTN_B, (text = "x"))
    t |> equal(v2, 42, "second call sees the mutated module global")
}
