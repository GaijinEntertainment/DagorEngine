options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

// Regression for issue #3311: a write performed by const-stripping (reinterpret to
// mutable) a `float const?` parameter must be recorded as a side effect even when the
// pointer reaches the writing leaf THROUGH a thin pass-through that also takes
// `float const?`. The leaf is correctly inferred [modify_argument]; the pass-through must
// inherit it. Before the fix the per-argument decision used the callee's declared (const)
// parameter type, so the forwarded write was invisible, the pass-through collapsed to
// [nosideeffects], and the whole call was dead-code-eliminated — silently dropping the
// write in interp, JIT and AOT alike. (Found while threading dasLLAMA prefill RoPE.)

// leaf: takes a const pointer, un-consts it, writes every element
def leaf(p : float const?; n : int) {
    unsafe {
        var w = reinterpret<float?>(p)
        for (i in range(n)) { w[i] = 7.0 }
    }
}

// thin pass-through forwarding the same `float const?` — the #3311 surface
def passthrough(p : float const?; n : int) {
    leaf(p, n)
}

// second pass-through chaining to the first — propagation must survive MORE than one
// `float const?` hop, not just collapse correctly at a single boundary
def passthrough2(p : float const?; n : int) {
    passthrough(p, n)
}

// control: mutable `var float?` params throughout, no reinterpret — always worked
def leaf_mut(var p : float?; n : int) {
    unsafe {
        for (i in range(n)) { p[i] = 7.0 }
    }
}
def passthrough_mut(var p : float?; n : int) {
    leaf_mut(p, n)
}

// pointer-alias shapes: a write through a local pointer variable initialized from a
// parameter must reach the parameter (else the function is judged pure and the call
// is DCE'd — same silent-drop family as #3311, via the let-alias hop instead of the
// const? forwarding). auto-inline generates exactly this binding for spliced args.
def passthrough_alias(p : float const?; n : int) {
    let q = p
    leaf(q, n)
}

def passthrough_alias2(p : float const?; n : int) {
    let q1 = p
    let q2 = q1
    leaf(q2, n)
}

def writer_via_copy(var p : float?; n : int) {
    var w = p
    unsafe {
        for (i in range(n)) { w[i] = 7.0 }
    }
}

def fill_zero(var a : float[4]) {
    for (i in range(4)) { a[i] = 0.0 }
}

[test]
def test_const_strip_write_through_passthrough(t : T?) {
    // (1) direct call to the writing leaf — was always correct
    t |> run("direct leaf write") @(t : T?) {
        var a : float[4]
        fill_zero(a)
        unsafe { leaf(addr(a[0]), 4) }
        t |> equal(a[0], 7.0)
    }
    // (2) forwarded through a `float const?` pass-through — the bug (b stayed 0.0)
    t |> run("write forwarded through const? pass-through") @(t : T?) {
        var b : float[4]
        fill_zero(b)
        unsafe { passthrough(addr(b[0]), 4) }
        t |> equal(b[0], 7.0)
    }
    // (3) forwarded through TWO chained `float const?` pass-throughs — transitive
    // propagation across multiple const? hops (each hop must inherit the write)
    t |> run("write forwarded through two const? hops") @(t : T?) {
        var c : float[4]
        fill_zero(c)
        unsafe { passthrough2(addr(c[0]), 4) }
        t |> equal(c[0], 7.0)
    }
    // (4) control: mutable-param pass-through — never lost the write
    t |> run("mutable-param pass-through control") @(t : T?) {
        var d : float[4]
        fill_zero(d)
        unsafe { passthrough_mut(addr(d[0]), 4) }
        t |> equal(d[0], 7.0)
    }
    // (5) write forwarded through a `let q = p` pointer-alias local
    t |> run("write through let-alias of const? param") @(t : T?) {
        var e : float[4]
        fill_zero(e)
        unsafe { passthrough_alias(addr(e[0]), 4) }
        t |> equal(e[0], 7.0)
    }
    // (6) two chained alias lets — chase must be transitive
    t |> run("write through chained let-aliases") @(t : T?) {
        var g : float[4]
        fill_zero(g)
        unsafe { passthrough_alias2(addr(g[0]), 4) }
        t |> equal(g[0], 7.0)
    }
    // (7) direct write through `var w = p` copy of a mutable param — no reinterpret at all
    t |> run("write through var-copy of mutable param") @(t : T?) {
        var h : float[4]
        fill_zero(h)
        unsafe { writer_via_copy(addr(h[0]), 4) }
        t |> equal(h[0], 7.0)
    }
}
