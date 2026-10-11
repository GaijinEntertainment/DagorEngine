options gen2
options indenting = 4

require math

//! Store-form corpus for `collect_mutable` (daslib/flatten_opt_common). One local per spelling a
//! store can take — plain copy, move, clone, increment, compound assign, swizzle lane, and a write
//! through a by-reference call argument — plus `ro`, the read-only control that must stay out of
//! the mutable set. The local names here are the assertion keys in test_flatten_mutcollect.das.
//!
//! `mut_out` is the signature side of the same corpus: one by-value parameter and one `var T&`,
//! so `writes_through_argument` can be asked a true case, a false case, and both out-of-range
//! ends against a real Function.

[export]
def mut_forms(x : float) : float {
    var cp = 0.0
    var mv : array<int>
    var cl : array<int>
    var src <- [1, 2, 3]
    var inc = 0
    var acc = 0.0
    var sn = 0.0
    var cs = 0.0
    var lane = float3(0.0)
    let ro = x * 2.0
    cp = x
    mv <- src
    cl := mv
    inc ++
    acc += x
    lane.y = ro
    sincos(x, sn, cs)
    return cp + acc + sn + cs + lane.y + ro + float(inc + length(mv) + length(cl))
}

[export]
def mut_out(x : float; var dst : float&) {
    dst = x * 2.0
}
