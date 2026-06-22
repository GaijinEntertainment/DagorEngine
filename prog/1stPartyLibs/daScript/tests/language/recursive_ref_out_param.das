options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

// Regression for the recursive `var T&` out-param miscompile (issue #3033).
// The write a recursive self-call performs through a writable-reference argument
// must be recorded by the access analysis, so a later r2v read of that local
// (condition / return / arithmetic) is NOT const-folded back to its initializer.

// direct recursion, result read in a condition
def rec_cond(n : int; var out : bool&) : bool {
    if (n <= 1) { out = true; return true }
    var inner = false
    let unused = rec_cond(n - 1, inner)
    if (inner) { return true } // nolint:STYLE017 — condition-read is the exact #3033 surface, must not collapse to `return inner`
    return false
}

// direct recursion, result read as a returned value
def rec_ret(n : int; var out : bool&) : bool {
    if (n <= 1) { out = true; return true }
    var inner = false
    let unused = rec_ret(n - 1, inner)
    return inner
}

// direct recursion, int out-param read in a condition
def rec_int(n : int; var acc : int&) : int {
    if (n <= 0) { acc = 100; return 100 }
    var inner = 0
    let unused = rec_int(n - 1, inner)
    if (inner == 100) { return inner }
    return -1
}

// mutual recursion through a cycle — both back-edge and forward-edge must record the write
def mr_a(n : int; var out : bool&) : bool {
    if (n <= 1) { out = true; return true }
    var inner = false
    let unused = mr_b(n - 1, inner)
    return inner
}
def mr_b(n : int; var out : bool&) : bool {
    if (n <= 1) { out = true; return true }
    var inner = false
    let unused = mr_a(n - 1, inner)
    return inner
}

// non-recursive control — was always correct; guards against over-eager folding
def leaf(x : int; var out : bool&) : bool { out = x > 0; return out }
def caller(x : int) : bool {
    var inner = false
    let unused = leaf(x, inner)
    if (inner) { return true } // nolint:STYLE017 — mirrors rec_cond's condition-read shape
    return false
}

[test]
def test_recursive_ref_out_param(t : T?) {
    t |> run("direct recursion, condition read") @(t : T?) {
        var b = false
        t |> success(rec_cond(2, b), "recursive write read in a condition")
    }
    t |> run("direct recursion, return read") @(t : T?) {
        var b = false
        t |> success(rec_ret(2, b), "recursive write read as a return value")
    }
    t |> run("direct recursion, int condition read") @(t : T?) {
        var a = 0
        t |> equal(rec_int(1, a), 100)
    }
    t |> run("mutual recursion") @(t : T?) {
        var b1 = false
        var b2 = false
        t |> success(mr_a(2, b1), "mutual recursion a->b")
        t |> success(mr_b(2, b2), "mutual recursion b->a")
    }
    t |> run("non-recursive control") @(t : T?) {
        t |> success(caller(5), "non-recursive write read in a condition")
    }
}
