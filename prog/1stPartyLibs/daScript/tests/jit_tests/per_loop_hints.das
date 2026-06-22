options gen2
require dastest/testing_boost

// Smoke tests for per-loop annotations on ExprFor / ExprWhile.
// These must parse and produce identical numeric results to their
// unannotated equivalents — the JIT lowering attaches LLVM loop
// metadata but does not change semantics.

def sum_range_bracketed(n : int) {
    var sum = 0
    for [unroll, vectorize] (i in range(1, n + 1)) {
        sum += i
    }
    return sum
}

def sum_range_metadata(n : int) {
    var sum = 0
    for @unroll_full(i in range(1, n + 1)) {
        sum += i
    }
    return sum
}

def sum_range_count(n : int) {
    var sum = 0
    for [unroll_count = 4] (i in range(1, n + 1)) {
        sum += i
    }
    return sum
}

def sum_range_bare(n : int) {
    var sum = 0
    for (i in range(1, n + 1)) {
        sum += i
    }
    return sum
}

def countdown_while_annotated(start : int) {
    var x = start
    var iters = 0
    while [unroll_disable] (x > 0) {
        x -= 1
        iters += 1
    }
    return iters
}

def countdown_while_bare(start : int) {
    var x = start
    var iters = 0
    while (x > 0) {
        x -= 1
        iters += 1
    }
    return iters
}

[test]
def test_per_loop_hints(t : T?) {
    t |> run("for [bracketed] matches bare") @(t : T?) {
        t |> equal(sum_range_bracketed(100), sum_range_bare(100))
    }
    t |> run("for @metadata matches bare") @(t : T?) {
        t |> equal(sum_range_metadata(100), sum_range_bare(100))
    }
    t |> run("for [unroll_count = N] matches bare") @(t : T?) {
        t |> equal(sum_range_count(100), sum_range_bare(100))
    }
    t |> run("while [unroll_disable] matches bare") @(t : T?) {
        t |> equal(countdown_while_annotated(50), countdown_while_bare(50))
    }
}
