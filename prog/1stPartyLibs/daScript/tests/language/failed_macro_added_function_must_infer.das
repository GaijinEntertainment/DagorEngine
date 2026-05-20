// regression test: a structure_macro that adds a function during
// patch() without setting astChanged used to leave the new function
// uninferred, which then crashed lint/checkSideEffects with a null
// expr->func deref.
//
// the compiler must turn this into a clean diagnostic instead of a
// crash. NoSideEffectVisitor and LintVisitor each catch the null
// func and emit error 50100 with a hint pointing at the missing
// astChanged=true in the macro.
options gen2
expect 50100:2    // call reached side-effect analysis / lint with null func

require _helper_macro_uninferred

[bad_emitter]
struct Foo {
    x : int
}

[export]
def main() {
}
