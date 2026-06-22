options gen2
options indenting = 4

require daslib/flatten

//! `[flatten]` unit for the optimize re-entry test — NOT a [test] file; it is compiled by
//! tests/flatten/no_aot/test_flatten_reentry.das, which then mutates the twin's body (the way a
//! backend rewrite would) and calls `flatten_optimize` a second time. The twin is shaped to keep
//! one `_preshader_0` let (the uniform `u` chain) and one `_cse_0` let (the duplicated varying
//! `(x + 1.0) * x`) in its final body, so a re-entered pass with un-seeded counters would mint
//! colliding names.

var G_REENTRY = 2.5

[flatten]
def reentry(x : float) : float {
    let u = G_REENTRY * 2.0 + 1.0
    let d = (x + 1.0) * x
    let e2 = (x + 1.0) * x
    return u * d + e2
}

[export]
def main() {
    print("{reentry_flat(3.0)}\n")
}
