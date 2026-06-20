options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/flatten
require math

//! Differential net for the mad disassemble → re-fuse arc and the DEFAULT lerp handling. The
//! fold expands `mad` calls into raw arithmetic (reassoc/CSE see through the operators); the
//! PHASE_FUSED finishing pass re-packs surviving `a*b ± c` into `mad(a,b,c)` and the lerp shape
//! `mad(b-a, t, a)` back into `lerp(a,b,t)`. `lerp` calls are KEPT WHOLE by default (a native-lerp
//! backend computes `(b-a)*t+a` in one instruction); the const-selector identities (`t=0`/`t=1`/
//! `a==b`) fire directly on the call. The opt-in expansion path (`options _flatten_expand_lerp`)
//! has its own corpus in test_flatten_lerp_expand.das.
//! Exactness per twin: int twins and same-builtin-both-sides round trips compare exact; ANY
//! float twin comparing raw interp ops against a `mad`/`lerp` builtin compares approx — the
//! vector builtins are hardware-fused (NEON FMLA / x86 FMADD), and even the scalar C++ impls
//! (`a*b + c`, `(b-a)*t + a`) get FMA-contracted by the host compiler, so the rounding is
//! platform-dependent. The FIRED structural proof (an actual `mad`/`lerp` call, no `a*b + c`
//! residual) lives in `no_aot/test_flatten_fold.das` via the fusion arms of
//! `flatten_opt_residuals`.

// Module globals are UNIFORM (preshader-eligible); function params are VARYING.
var G_A = 0.25
var G_B = 4.0

// ----- mad fusion: plain mul-add, both operand orders -----
[flatten]
def m_basic(a : float; b : float; c : float) : float {
    return a * b + c
}

[flatten]
def m_right(a : float; b : float; c : float) : float {
    return c + a * b
}

// ----- the shader remap idiom x*C + C -----
[flatten]
def m_remap(x : float) : float {
    return x * 0.5 + 0.5
}

// ----- `-` forms: const-negatable addend / const factor -----
[flatten]
def m_subc(a : float; b : float) : float {
    return a * b - 1.0
}

[flatten]
def m_csub(b : float) : float {
    return 1.0 - 2.0 * b
}

// ----- two muls: greedy fuse absorbs one, the other stays mad's addend -----
[flatten]
def m_chain(a : float; b : float; c : float; d : float) : float {
    return a * b + c * d
}

// ----- int mad (exact in every tier) -----
[flatten]
def m_int(a : int; b : int; c : int) : int {
    return a * b + c
}

// ----- vector mad + the MadS scalar-broadcast form (hardware-fused -> approx) -----
[flatten]
def m_vec(a : float3; b : float3; c : float3) : float3 {
    return a * b + c
}

[flatten]
def m_vecs(v : float3; s : float; w : float3) : float3 {
    return v * s + w
}

// ----- int vector mad + int MadS (exact) -----
[flatten]
def m_veci(a : int3; b : int3; c : int3) : int3 {
    return a * b + c
}

[flatten]
def m_intvs(v : int3; s : int; w : int3) : int3 {
    return v * s + w
}

// ----- lerp const-selector short-circuits -----
[flatten]
def l_t0(a : float; b : float) : float {
    return lerp(a, b, 0.0)
}

[flatten]
def l_t1(a : float; b : float) : float {
    return lerp(a, b, 1.0)
}

[flatten]
def l_same(a : float; t : float) : float {
    return lerp(a, a, t)
}

// ----- lerp with a const selector hitting the t=1 arm directly on the call -----
[flatten]
def l_const1(d : float) : float {
    return lerp(1.0, d, 1.0)
}

// ----- plain lerp: survives as the same call (no expansion by default) -----
[flatten]
def l_round(a : float; b : float; t : float) : float {
    return lerp(a, b, t)
}

[flatten]
def l_vec(a : float3; b : float3; t : float) : float3 {
    return lerp(a, b, t)
}

// ----- raw arithmetic canonicalized INTO lerp (never spelled `lerp` in source) -----
[flatten]
def l_manual(a : float; b : float; t : float) : float {
    return (b - a) * t + a
}

[flatten]
def l_vec_manual(a : float3; b : float3; t : float) : float3 {
    return (b - a) * t + a
}

// ----- uniform lerp bounds: the call survives whole, bounds ride as const operands -----
[flatten]
def p_lerp(x : float) : float {
    return lerp(G_A, G_B, x)
}

var GRID : array<float>

[init]
def fill_grid {
    GRID <- [-100.0, -3.5, -1.0, 0.0, 1.0, 2.5, 7.0, 100.0]
}

def fapprox(t : T?; a, b : float) {
    let m = max(max(abs(a), abs(b)), 1.0)
    t |> success(abs(a - b) <= 1.0e-4 * m, "fused-mad approx mismatch: {a} vs {b}")
}

def cmp3f_approx(t : T?; a, b : float3) {
    fapprox(t, a.x, b.x); fapprox(t, a.y, b.y); fapprox(t, a.z, b.z)
}

def cmp3i(t : T?; a, b : int3) {
    t |> equal(a.x, b.x); t |> equal(a.y, b.y); t |> equal(a.z, b.z)
}

[test]
def test_flatten_mad(t : T?) {
    t |> run("mad: a*b + c fuses (approx)") @(t : T?) {
        for (v in GRID) {
            fapprox(t, m_basic_flat(v, v * 0.5 - 1.0, 3.0), m_basic(v, v * 0.5 - 1.0, 3.0))
        }
    }
    t |> run("mad: c + a*b fuses (approx)") @(t : T?) {
        for (v in GRID) {
            fapprox(t, m_right_flat(v, v + 2.0, -v), m_right(v, v + 2.0, -v))
        }
    }
    t |> run("mad: x*C + C remap (approx)") @(t : T?) {
        for (v in GRID) {
            fapprox(t, m_remap_flat(v), m_remap(v))
        }
    }
    t |> run("mad: a*b - C negates the addend (approx)") @(t : T?) {
        for (v in GRID) {
            fapprox(t, m_subc_flat(v, v - 3.0), m_subc(v, v - 3.0))
        }
    }
    t |> run("mad: C - a*b negates the const factor (approx)") @(t : T?) {
        for (v in GRID) {
            fapprox(t, m_csub_flat(v), m_csub(v))
        }
    }
    t |> run("mad: a*b + c*d absorbs one mul (approx)") @(t : T?) {
        for (v in GRID) {
            fapprox(t, m_chain_flat(v, 2.0, v - 1.0, 3.0), m_chain(v, 2.0, v - 1.0, 3.0))
        }
    }
    t |> run("mad: int (exact)") @(t : T?) {
        for (v in GRID) {
            let n = int(v)
            t |> equal(m_int_flat(n, n + 3, 7), m_int(n, n + 3, 7))
        }
    }
    t |> run("mad: float3 (approx — hardware-fused)") @(t : T?) {
        for (v in GRID) {
            let a = float3(v, -v, v * 0.5)
            cmp3f_approx(t, m_vec_flat(a, a + float3(1.0), a - float3(2.0)), m_vec(a, a + float3(1.0), a - float3(2.0)))
        }
    }
    t |> run("mad: float3 * scalar + float3 MadS (approx)") @(t : T?) {
        for (v in GRID) {
            let w = float3(v, v + 1.0, -v)
            cmp3f_approx(t, m_vecs_flat(w, v * 0.25, w * 2.0), m_vecs(w, v * 0.25, w * 2.0))
        }
    }
    t |> run("mad: int3 (exact)") @(t : T?) {
        for (v in GRID) {
            let n = int(v)
            let a = int3(n, n + 1, -n)
            cmp3i(t, m_veci_flat(a, a + int3(2), a - int3(3)), m_veci(a, a + int3(2), a - int3(3)))
        }
    }
    t |> run("mad: int3 * scalar + int3 MadS (exact)") @(t : T?) {
        for (v in GRID) {
            let n = int(v)
            let a = int3(n, n - 2, n + 5)
            cmp3i(t, m_intvs_flat(a, n + 1, a * 2), m_intvs(a, n + 1, a * 2))
        }
    }
    t |> run("lerp: t=0 -> a (exact)") @(t : T?) {
        for (v in GRID) {
            t |> equal(l_t0_flat(v, v * 3.0), l_t0(v, v * 3.0))
        }
    }
    t |> run("lerp: t=1 -> b (approx — twin computes a + (b-a))") @(t : T?) {
        for (v in GRID) {
            fapprox(t, l_t1_flat(v, v * 3.0 + 1.0), l_t1(v, v * 3.0 + 1.0))
        }
    }
    t |> run("lerp: a==b -> a (exact)") @(t : T?) {
        for (v in GRID) {
            t |> equal(l_same_flat(v, v * 0.5), l_same(v, v * 0.5))
        }
    }
    t |> run("lerp: lerp(1, d, 1) folds to d (exact)") @(t : T?) {
        for (v in GRID) {
            t |> equal(l_const1_flat(v), l_const1(v))
        }
    }
    t |> run("lerp: scalar round trip (exact — same builtin both sides)") @(t : T?) {
        for (v in GRID) {
            t |> equal(l_round_flat(v, v + 5.0, 0.25), l_round(v, v + 5.0, 0.25))
        }
    }
    t |> run("lerp: vector round trip (exact — same builtin both sides)") @(t : T?) {
        for (v in GRID) {
            let a = float3(v, -v, v + 1.0)
            let b = a * 2.0 + float3(1.0)
            t |> equal(l_vec_flat(a, b, 0.75).x, l_vec(a, b, 0.75).x)
            t |> equal(l_vec_flat(a, b, 0.75).y, l_vec(a, b, 0.75).y)
            t |> equal(l_vec_flat(a, b, 0.75).z, l_vec(a, b, 0.75).z)
        }
    }
    t |> run("lerp: raw (b-a)*t + a canonicalizes INTO lerp (approx)") @(t : T?) {
        for (v in GRID) {
            fapprox(t, l_manual_flat(v, v * 2.0 - 3.0, 0.4), l_manual(v, v * 2.0 - 3.0, 0.4))
        }
    }
    t |> run("lerp: raw vector form -> lerp (approx — builtin is fused)") @(t : T?) {
        for (v in GRID) {
            let a = float3(v, v * 0.5, -v)
            let b = float3(-v, v + 2.0, v)
            cmp3f_approx(t, l_vec_manual_flat(a, b, 0.3), l_vec_manual(a, b, 0.3))
        }
    }
    t |> run("lerp: uniform bounds -> preshader split + mad (exact)") @(t : T?) {
        for (v in GRID) {
            t |> equal(p_lerp_flat(v), p_lerp(v))
        }
    }
}
