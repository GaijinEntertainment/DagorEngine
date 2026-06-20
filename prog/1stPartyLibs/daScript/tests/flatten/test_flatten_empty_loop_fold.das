// Regression: a [flatten] function calling a pure helper whose for-over-const-array
// loop body DSE's to empty used to SIGSEGV the constant-folder — it simulated the
// fused single-statement for-node (`loop1`, chosen from the AST statement count) with a
// null body list, then read list[0]. Compiling this file is the test; the differential
// confirms the flattened twin still matches the pristine original.
options gen2
options indenting = 4
options no_unused_function_arguments = false
require dastest/testing_boost public
require daslib/flatten

def helper_e(p0, p1 : bool) : bool {
    for (_i in [-1, -2, 4]) {
        var v = ((!false) != (p1 || false))   // nolint:LINT002 -- intentional dead store: the single body statement that DSE's to 0 SimNodes (the crash trigger)
    }
    return (!(!p0))
}

[flatten]
def cand_e(a, b, _c : bool) : bool {
    if (((!b) == (false && b))) {
        var v225 = helper_e(((false || true) && true), (!(!false)))
        v225 = (v225 != (false || a))
    }
    return false
}

[test]
def test_flatten_empty_loop_fold(t : T?) {
    for (a in [false, true]) {
        for (b in [false, true]) {
            for (c in [false, true]) {
                t |> equal(cand_e(a, b, c), cand_e_flat(a, b, c))
            }
        }
    }
}
