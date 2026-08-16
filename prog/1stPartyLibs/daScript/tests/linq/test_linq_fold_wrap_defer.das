options gen2
require daslib/linq
require dastest/testing_boost public

require daslib/linq_boost

// Wrapping form `_fold(chain)` must defer until the chain's call-site type resolves. `select`'s declared
// return type is the dependent `array<typedecl(result_selector(type<TT>))>`, which only collapses once its
// generic instance finishes inferring; firing in that window baked the unresolved typedecl into the emitted
// buffer decl (error[30341] on result_selector). The dot form `chain._fold()` dodged it by promotion timing
// only. This file stays minimal ON PURPOSE: the window opens only for a FRESH `select` instance, so any other
// chain in the program that warms the same instantiation masks the regression (test_linq_fold.das never
// reproduced it).

[test]
def test_fold_wrapping_form_defers(t : T?) {
    t |> run("wrap form, array head, select last") @(tt : T?) {
        var r <- _fold([1, 2, 3, 4, 5]
            ._where(_ <= 3)
            ._select(_ * 2))
        tt |> equal(typeinfo typename(r), "array<int>")
        for (i, v in 0..3, r) {
            tt |> equal((i + 1) * 2, v)
        }
    }
    t |> run("wrap form, iterator head, select last") @(tt : T?) {
        var r <- _fold([1, 2, 3, 4, 5].to_sequence()
            ._where(_ <= 3)
            ._select(_ * 2))
        tt |> equal(typeinfo typename(r), "iterator<int>")
        for (i, v in 0..3, r) {
            tt |> equal((i + 1) * 2, v)
        }
    }
}
