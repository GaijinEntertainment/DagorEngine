options gen2

require dastest/testing_boost public

// AOT most-vexing-parse regression: a for-source made ONLY of constructor-style casts
// (range64(int64(a), int64(b))) used to emit
//   das_iterator<range64> it(range64(int64_t(a),int64_t(b)));
// which C++ parses as a function DECLARATION, not a variable. The emitter now
// double-parenthesizes the initializer.

// mutable globals so the calls can't const-fold — the loop must survive to AOT
var private g_lo = 3
var private g_hi = 7

def private sum_range(tb, te : int) : int64 {
    var s = 0l
    for (t in range64(int64(tb), int64(te))) {
        s += t
    }
    return s
}

[test]
def test_range64_ctor_arg_source(t : T?) {
    t |> equal(18l, sum_range(g_lo, g_hi))
    t |> equal(0l, sum_range(g_hi, g_hi))
}
