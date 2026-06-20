options gen2
options indenting = 4
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/flatten

//! Nested-if differential tests for daslib/flatten. Each twin is run against its
//! original over a 2-D input grid AND (via the macro's cycle-2 scan) proven
//! branchless + call-free. Covers the predicate-AND threading (`structPred && c` /
//! `structPred && !c` recursed through nesting) and the live-mask interplay when an
//! early return sits inside a nested branch.

// ----- if nested in the then-branch (no else anywhere): single deep predicate -----
// Lowers to one select gated on `(x>0) && (y>0)`.
[flatten]
def f_then_then(x : int; y : int) : int {
    var r = 0
    if (x > 0) {
        if (y > 0) {
            r = 1
        }
    }
    return r
}

// ----- if nested in the else-branch -----
// `if (x>0) r=1 else { if (y>0) r=2 }` -> selects gated on `x>0` and `!(x>0) && (y>0)`.
[flatten]
def f_else_then(x : int; y : int) : int {
    var r = 0
    if (x > 0) {
        r = 1
    } else {
        if (y > 0) {
            r = 2
        }
    }
    return r
}

// ----- if/else nested in BOTH arms (all four quadrants) -----
// Lowers to four selects, one per quadrant: (x>0)&&(y>0), (x>0)&&!(y>0),
// !(x>0)&&(y>0), !(x>0)&&!(y>0).
[flatten]
def f_quad(x : int; y : int) : int {
    var r = 0
    if (x > 0) {
        if (y > 0) {
            r = 1
        } else {
            r = 2
        }
    } else {
        if (y > 0) {
            r = 3
        } else {
            r = 4
        }
    }
    return r
}

// ----- triple nesting (if in if in if) -----
[flatten]
def f_triple(x : int; y : int; z : int) : int {
    var r = 0
    if (x > 0) {
        if (y > 0) {
            if (z > 0) {
                r = 7
            }
        }
    }
    return r
}

// ----- early return inside a nested if (live-mask threading) -----
// `__flat_live` tracks "has a return fired"; the trailing return is gated on it.
[flatten]
def f_nested_ret(x : int; y : int) : int {
    if (x > 0) {
        if (y > 0) {
            return 10
        }
        return 20
    }
    return 30
}

// ----- both inner arms return, outer falls through (no inner fallthrough) -----
[flatten]
def f_both_ret(x : int; y : int) : int {
    if (x > 0) {
        if (y > 0) {
            return 1
        } else {
            return 2
        }
    }
    return 3
}

// ----- nested predicate over a SHARED accumulator (compound-assign under nesting) -----
// Mixes early-return live mask, structural predicate, and `+=` lowering in one body.
[flatten]
def f_nested_accum(x : int; y : int) : int {
    var acc = 0
    if (x > 0) {
        acc ++                    // ++ under a structural predicate
        if (y > 0) {
            return acc + 100      // early return out of the nested then
        }
        acc += 2
    }
    acc += 3
    return acc
}

// ----- driver -----

var GRID : array<int>

[init]
def fill_grid {
    GRID <- [-5, -1, 0, 1, 5]
}

[test]
def test_flatten_nested(t : T?) {
    t |> run("if nested in then (deep AND predicate)") @(t : T?) {
        for (a in GRID) {
            for (b in GRID) {
                t |> equal(f_then_then_flat(a, b), f_then_then(a, b))
            }
        }
    }
    t |> run("if nested in else") @(t : T?) {
        for (a in GRID) {
            for (b in GRID) {
                t |> equal(f_else_then_flat(a, b), f_else_then(a, b))
            }
        }
    }
    t |> run("if/else in both arms (four quadrants)") @(t : T?) {
        for (a in GRID) {
            for (b in GRID) {
                t |> equal(f_quad_flat(a, b), f_quad(a, b))
            }
        }
    }
    t |> run("triple nesting") @(t : T?) {
        for (a in GRID) {
            for (b in GRID) {
                for (c in GRID) {
                    t |> equal(f_triple_flat(a, b, c), f_triple(a, b, c))
                }
            }
        }
    }
    t |> run("early return inside nested if (live mask)") @(t : T?) {
        for (a in GRID) {
            for (b in GRID) {
                t |> equal(f_nested_ret_flat(a, b), f_nested_ret(a, b))
            }
        }
    }
    t |> run("both inner arms return") @(t : T?) {
        for (a in GRID) {
            for (b in GRID) {
                t |> equal(f_both_ret_flat(a, b), f_both_ret(a, b))
            }
        }
    }
    t |> run("nested predicate over a shared accumulator") @(t : T?) {
        for (a in GRID) {
            for (b in GRID) {
                t |> equal(f_nested_accum_flat(a, b), f_nested_accum(a, b))
            }
        }
    }
}
