// Module that consumes the [dyn_register_42] annotation from the helper.
// At apply-time, registers a call_macro named "answer" that folds any
// `answer(N)` call to the constant 42. The function body is intentionally
// `return x` (i.e. NOT 42) — if the macro dispatch regresses and the
// original function runs, the test sees x and fails loud.
//
// Has to live in a separate file from the meta-macro class declaration
// (per the cross-file visibility gotcha — see _dyn_register_helper.das).
options gen2
options indenting = 4

module _dyn_register_decl public

require _dyn_register_helper public

[dyn_register_42]
def public answer(x : int) : int {
    return x
}
