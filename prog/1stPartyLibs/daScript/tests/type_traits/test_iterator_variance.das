options gen2
require dastest/testing_boost public

// Iterator element-const variance is pointer-like: `iterator<T -&>` flows into
// `iterator<T const -&>`, but not the reverse. Tested empirically while wiring
// the linq.das constify fix (commit 0851ea03e). These tests pin the rule so
// future language regressions surface as test failures rather than as a fresh
// generic-mangler ICE in user code.

def private sum_const_iter(var src : iterator<int const&>) : int {
    var total = 0
    for (it in src) {
        total += it
    }
    return total
}

def private sum_const_generic(var src : iterator<auto(TT) const&>) : int {
    var total = 0
    for (it in src) {
        total += it
    }
    return total
}

def private sum_plain_generic(var src : iterator<auto(TT)>) : int {
    var total = 0
    for (it in src) {
        total += it
    }
    return total
}

[test]
def test_mut_iter_flows_into_const_param(t : T?) {
    // each(array<int>) yields iterator<int -&> (mut element). Passing it into a
    // function expecting iterator<int const -&> should succeed via variance.
    let arr <- [1, 2, 3, 4, 5]
    unsafe {
        let total = sum_const_iter(each(arr))
        t |> equal(15, total)
    }
}

[test]
def test_const_source_into_const_param(t : T?) {
    // Iterator comprehension yields iterator<int const -&>. Passing it into a
    // function expecting iterator<int const -&> is the trivial case.
    let total = sum_const_iter([iterator for(x in 1..6); x])
    t |> equal(15, total)
}

[test]
def test_generic_const_param_accepts_both_flavors(t : T?) {
    // The regression case: the same generic instantiated from BOTH a mut-element
    // source AND a const-element source within one module. Pre-fix this would
    // ICE with `error[50609]: multiple instances of …` because the mangler
    // ignored inner element-const. Post-fix both flavors converge on a single
    // instance (TT = int) via variance, no collision.
    let arr <- [10, 20, 30]
    unsafe {
        let from_mut   = sum_const_generic(each(arr))
        let from_const = sum_const_generic([iterator for(x in 100..103); x])
        t |> equal(60, from_mut)
        t |> equal(303, from_const)
    }
}

[test]
def test_plain_generic_flavor_siblings_no_ice(t : T?) {
    // A PLAIN-element generic mints one instance per element flavor (mut, then
    // const). Pre-fix a THIRD mut-source call ICE'd with `error[50609]: multiple
    // instances of ...` — the instance re-lookup matched both siblings through
    // element variance with no exact-flavor tie-break. Order matters: mut first.
    let arr <- [1, 2, 3]
    unsafe {
        t |> equal(6, sum_plain_generic(each(arr)))
        t |> equal(33, sum_plain_generic([iterator for(x in 10..13); x]))
        t |> equal(6, sum_plain_generic(each(arr)))
    }
}
