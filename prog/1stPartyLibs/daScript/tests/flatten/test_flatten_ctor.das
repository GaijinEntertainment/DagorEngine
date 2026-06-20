options gen2
options indenting = 4
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/flatten
require math

//! Twin corpus for the ctor lane algebra. Fold phase: zero-lane kill (`floatN(e…) * constVec`
//! zeroes the ctor lanes under a zero const lane — fast-math for floats, exact for ints) and
//! splat collapse (`v ⊙ floatN(s) → v ⊙ s` for * and / — bit-exact). Fuse phase: lane
//! re-vectorization (`floatN(v.x + c1, v.y + c2, …) → v + floatN(c…)` — bit-exact, the per-lane
//! ops are the same IEEE ops), shared-factor extraction (`floatN(s*a, s*b, s*c) → floatN(a,b,c)
//! * s` — bit-exact), and majority-const compensation (`float3(x*C1, y*C1, z*C2) → float3(x, y,
//! z*(C2/C1)) * C1` — the odd-lane reciprocal rounds → fapprox + fast-math gate). All exact
//! twins compare `equal`; only the compensation twin needs `fapprox`. Structural halves live in
//! tests/flatten/no_aot/test_flatten_fold.das (option-aware oracles) and capability/cap_ctor.shader.

var G_CTOR = 2.5

// ----- fold phase: zero-lane kill + splat collapse -----

[flatten]
def z_kill(x : float) : float2 {
    return float2(sin(x) * 12.0, 13.0) * float2(0.0, 123.0)
}

[flatten]
def z_partial(x, y : float) : float3 {
    return float3(sin(x), y * 2.0, 5.0) * float3(0.0, 2.0, 3.0)
}

[flatten]
def z_int(n : int) : int2 {
    return int2(n * 7, 5) * int2(0, 3)
}

[flatten]
def z_uint(n : uint) : uint2 {
    return uint2(n * 7u, 5u) * uint2(0u, 3u)
}

[flatten]
def sp_mul(v : float3; s : float) : float3 {
    return v * float3(s)
}

[flatten]
def sp_div(v : float3; s : float) : float3 {
    return v / float3(s)
}

[flatten]
def sp_rdiv(v : float3; s : float) : float3 {
    return float3(s) / v
}

// ----- fuse phase: lane re-vectorization -----

[flatten]
def rv_add(v : float3) : float3 {
    return sin(float3(v.x + 0.1, v.y + 0.2, v.z + 0.3))
}

[flatten]
def rv_swiz(v : float4) : float3 {
    return float3(v.z + 1.0, v.x + 2.0, v.y + 3.0)
}

[flatten]
def rv_gather(v : float2; s1, s2 : float) : float2 {
    return float2(v.x * s1, v.y * s2)
}

[flatten]
def rv_eq_add(v : float3; s : float) : float3 {
    return float3(s + v.x, s + v.y, s + v.z)
}

// ----- fuse phase: shared factor / majority compensation -----

[flatten]
def sf_var(x : float) : float3 {
    let g = x * 0.5
    return float3(g * 1.08, g * 0.86, g * 0.66)
}

[flatten]
def sf_const(x, y, z : float) : float3 {
    return float3(x * 13.0, y * 13.0, z * 13.0)
}

[flatten]
def sf_div(x, y, z, s : float) : float3 {
    return float3(x / s, y / s, z / s)
}

[flatten]
def mj_comp(x, y, z : float) : float3 {
    return float3(x * 13.0, y * 13.0, z * 12.0)
}

// ----- differential driver -----

var GRID : array<float>
var IGRID : array<int>

[init]
def fill_grid {
    GRID <- [-100.0, -3.5, -1.0, -0.0, 0.0, 1.0, 2.5, 7.0, 100.0]
    IGRID <- [-100, -7, -1, 0, 1, 3, 42, 1000]
}

def fapprox(t : T?; a, b : float) {
    let m = max(max(abs(a), abs(b)), 1.0)
    t |> success(abs(a - b) <= 1.0e-4 * m, "ctor approx mismatch: {a} vs {b}")
}

def cmp2f(t : T?; a, b : float2) {
    t |> equal(a.x, b.x); t |> equal(a.y, b.y)
}

def cmp3f(t : T?; a, b : float3) {
    t |> equal(a.x, b.x); t |> equal(a.y, b.y); t |> equal(a.z, b.z)
}

def cmp3f_approx(t : T?; a, b : float3) {
    fapprox(t, a.x, b.x); fapprox(t, a.y, b.y); fapprox(t, a.z, b.z)
}

[test]
def test_flatten_ctor(t : T?) {
    t |> run("zero-lane kill collapses the whole product (exact)") @(t : T?) {
        for (v in GRID) {
            cmp2f(t, z_kill_flat(v), z_kill(v))
        }
    }
    t |> run("partial zero-lane kill (exact)") @(t : T?) {
        for (v in GRID) {
            cmp3f(t, z_partial_flat(v, -v), z_partial(v, -v))
        }
    }
    t |> run("int zero-lane kill (exact, ungated)") @(t : T?) {
        for (n in IGRID) {
            let r = z_int_flat(n)
            let o = z_int(n)
            t |> equal(r.x, o.x)
            t |> equal(r.y, o.y)
        }
    }
    t |> run("uint zero-lane kill (exact, ungated)") @(t : T?) {
        for (n in IGRID) {
            let u = uint(n)
            let r = z_uint_flat(u)
            let o = z_uint(u)
            t |> equal(r.x, o.x)
            t |> equal(r.y, o.y)
        }
    }
    t |> run("v * splat(s) -> v * s (exact)") @(t : T?) {
        for (v in GRID) {
            cmp3f(t, sp_mul_flat(float3(v, -v, 1.0), 3.5), sp_mul(float3(v, -v, 1.0), 3.5))
        }
    }
    t |> run("v / splat(s) -> v / s (exact)") @(t : T?) {
        for (v in GRID) {
            cmp3f(t, sp_div_flat(float3(v, -v, 1.0), 3.5), sp_div(float3(v, -v, 1.0), 3.5))
        }
    }
    t |> run("splat(s) / v -> s / v (exact)") @(t : T?) {
        for (v in GRID) {
            cmp3f(t, sp_rdiv_flat(float3(v, -v, 1.0), 3.5), sp_rdiv(float3(v, -v, 1.0), 3.5))
        }
    }
    t |> run("re-vectorize per-component adds (exact)") @(t : T?) {
        for (v in GRID) {
            cmp3f(t, rv_add_flat(float3(v, -v, 1.0)), rv_add(float3(v, -v, 1.0)))
        }
    }
    t |> run("re-vectorize through a permuted swizzle (exact)") @(t : T?) {
        for (v in GRID) {
            cmp3f(t, rv_swiz_flat(float4(v, -v, 1.0, 2.0)), rv_swiz(float4(v, -v, 1.0, 2.0)))
        }
    }
    t |> run("re-vectorize with a gathered varying side (exact)") @(t : T?) {
        for (v in GRID) {
            cmp2f(t, rv_gather_flat(float2(v, -v), 3.0, 5.0), rv_gather(float2(v, -v), 3.0, 5.0))
        }
    }
    t |> run("re-vectorize with an equal-scalar splat side (exact)") @(t : T?) {
        for (v in GRID) {
            cmp3f(t, rv_eq_add_flat(float3(v, -v, 1.0), 2.25), rv_eq_add(float3(v, -v, 1.0), 2.25))
        }
    }
    t |> run("shared var factor -> const-vec * s (exact)") @(t : T?) {
        for (v in GRID) {
            cmp3f(t, sf_var_flat(v), sf_var(v))
        }
    }
    t |> run("shared const factor -> ctor * C (exact)") @(t : T?) {
        for (v in GRID) {
            cmp3f(t, sf_const_flat(v, -v, 1.0), sf_const(v, -v, 1.0))
        }
    }
    t |> run("shared divisor -> ctor / s (exact)") @(t : T?) {
        for (v in GRID) {
            cmp3f(t, sf_div_flat(v, -v, 1.0, 3.5), sf_div(v, -v, 1.0, 3.5))
        }
    }
    t |> run("majority const factor compensates the odd lane (approx)") @(t : T?) {
        for (v in GRID) {
            cmp3f_approx(t, mj_comp_flat(v, -v, 1.0), mj_comp(v, -v, 1.0))
        }
    }
}
