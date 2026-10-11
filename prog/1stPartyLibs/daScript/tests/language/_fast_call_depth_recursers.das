options gen2
options no_aot    // the guard counts interpreted fastcall nodes - an AOT body recurses natively into the stack guard page
module _fast_call_depth_recursers shared

//! Every recurser is [no_jit] for the same reason the module keeps out of AOT: a jitted body is native.

var g_sink = 0       //! every probe result lands here, so no call is dropped as a pure call with an unused result
var g_no_fold = 0    //! added to every argument, so no probe folds to a constant at compile time

[no_jit]
def rec_forever(n : int) : int {
    return rec_forever(n + 1)
}

[no_jit]
def rec_sum(n : int) : int {
    return n == 0 ? 0 : n + rec_sum(n - 1)
}

//! both arguments simple: the fused ArgConst shape
[no_jit]
def rec_sum2(n : int, acc : int) : int {
    return n == 0 ? acc : rec_sum2(n - 1, acc + n)
}

//! a computed left argument beside a plain one: the fused Any-left shape
[no_jit]
def rec_any_left(n : int, acc : int) : int {
    return n == 0 ? acc : rec_any_left(n - 1, acc)
}

//! a plain left argument beside a computed one: the fused Any-right shape
[no_jit]
def rec_any_right(acc : int, n : int) : int {
    return n == 0 ? acc : rec_any_right(acc, n - 1)
}

//! nine arguments: the unrolled node family stops at eight, past it the variadic specialization
[no_jit]
def rec_nine(a, b, c, d, e, f, g, h : int; n : int) : int {
    return n == 0 ? a + b + c + d + e + f + g + h : rec_nine(a, b, c, d, e, f, g, h, n - 1)
}

//! A framed, interpreted entry: the counted chain starts at the recursion itself, so the depth
//! is n + 1 whether the caller is interpreted, jitted or AOT.
[export, never_inline, no_jit]
def framed_rec_sum(n : int) : int {
    return rec_sum(n)
}

[export, never_inline, no_jit]
def framed_rec_sum2(n : int) : int {
    return rec_sum2(n, 0)
}

[export, never_inline, no_jit]
def framed_rec_any_left(n : int) : int {
    return rec_any_left(n, 7)
}

[export, never_inline, no_jit]
def framed_rec_any_right(n : int) : int {
    return rec_any_right(7, n)
}

[export, never_inline, no_jit]
def framed_rec_nine(n : int) : int {
    return rec_nine(1, 2, 3, 4, 5, 6, 7, 8, n)
}

def triangular(n : int) : int {
    return n * (n + 1) / 2
}
