options gen2
require dastest/testing_boost

[sideeffects]
def test_const_r64 {
    return range64(1, 2)
}

[sideeffects]
def test_const_ur64 {
    return urange64(3, 4)
}

[sideeffects]
def test_make_r64_from_int(a : int) {
    return range64(a)
}

[sideeffects]
def test_make_r64_from_2xr64(a, b : int64) {
    return range64(a, b)
}

[sideeffects]
def test_make_r64_from_2xfloat(a, b : float) {
    return range64(a, b)
}

[sideeffects]
def test_r64_yx(r : range64) : range64 {
    return r.yx
}

[sideeffects]
def test_for_range64 {
    var t = 0ul
    for (x in 0ul .. 10ul) {
        t ++
    }
    return t
}

[test]
def test_range64_urange64_intrinsics(t : T?) {
    t |> run("constants") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_const_r64))
        t |> equal(range64(1, 2), test_const_r64())
        t |> success(!jit_enabled() || is_jit_function(@@test_const_ur64))
        t |> equal(urange64(3, 4), test_const_ur64())
    }
    t |> run("constructors") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_make_r64_from_int))
        t |> equal(range64(0, 123), test_make_r64_from_int(123))
        t |> success(!jit_enabled() || is_jit_function(@@test_make_r64_from_2xr64))
        let c1 = 1l << 32l
        let c2 = 1l << 33l
        t |> equal(range64(c1, c2), test_make_r64_from_2xr64(c1, c2))
        t |> success(!jit_enabled() || is_jit_function(@@test_make_r64_from_2xfloat))
        t |> equal(range64(1., 2.), test_make_r64_from_2xfloat(1., 2.))
    }
    t |> run("swizzle") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_r64_yx))
        t |> equal(range64(2, 1), test_r64_yx(range64(1, 2)))
    }
    t |> run("for") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@test_for_range64))
        t |> equal(10ul, test_for_range64())
    }
}

[test]
def test_urange64_vector_access(t : T?) {
    t |> run("heap_allocation_stats index and swizzle") @(t : T?) {
        let hpbs = heap_allocation_stats()
        t |> equal(hpbs[0], heap_allocation_stats()[0])
        t |> equal(hpbs[1], heap_allocation_stats()[1])
        t |> equal(hpbs.x, heap_allocation_stats().x)
        t |> equal(hpbs.y, heap_allocation_stats().y)
    }
}

