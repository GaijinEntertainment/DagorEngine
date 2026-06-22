options gen2
options indenting = 4
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/flatten
require math

//! Verification net for the flatten const-optimization arc — two layers per case
//! (numbered per the arc's 5-layer verification framework; Layer 1, the randomized
//! fold-oracle, is deferred to a later arc, hence the 0/2 gap):
//!   • VALUE (Layer 0): each `[flatten]` twin must compute identical results to its
//!     un-flattened original over a FINITE input grid. On this grid the lossy float
//!     folds (`x*0`, `x+0`, `x-0`) preserve `==` equality (`finite*0 == 0`, `x+0 == x`;
//!     the bitwise `-0.0`/NaN/inf differences either don't arise here or compare
//!     `==`-equal), so plain `equal` suffices — `equal_approx` is reserved for the
//!     reassociation arc, where rounding genuinely differs.
//!   • FIRED (Layer 2): differential alone can't see a fold that failed to fire — an
//!     un-optimized twin computes the same value, so VALUE checks must be paired with a
//!     structural proof that the collapse happened. That proof lives in
//!     `tests/flatten/no_aot/test_flatten_fold.das`, which compiles THIS file as a unit
//!     and asserts `flatten_fold_residuals` is empty on every `*_flat` twin (rejecting any
//!     unfolded algebraic identity, all-const call, or const-condition select left behind).
//!     (Node-count proof at the backend level lives in the cap_fold_* shaders.) The
//!     genuinely-non-folding shapes here — a param-seeded runtime accumulator, or a
//!     scalar-broadcast `s * vec(1)` the same_base_type gate must NOT collapse to scalar —
//!     are part of that same corpus: their empty residual set proves the check does NOT
//!     false-positive on a shape that legitimately does not fold.

// ----- float algebraic identities (#1a) -----

[flatten]
def f_mul1(x : float) : float {
    return x * 1.0
}

[flatten]
def f_mul1l(x : float) : float {
    return 1.0 * x
}

[flatten]
def f_mulneg1(x : float) : float {
    return x * -1.0
}

[flatten]
def f_mul0(x : float) : float {
    return x * 0.0
}

[flatten]
def f_add0(x : float) : float {
    return x + 0.0
}

[flatten]
def f_sub0(x : float) : float {
    return x - 0.0
}

[flatten]
def f_zerosub(x : float) : float {
    return 0.0 - x
}

// Chained identities collapse bottom-up: (x*1) + (x*0) - 0 -> x + 0 - 0 -> x.
[flatten]
def f_combo(x : float) : float {
    return (x * 1.0) + (x * 0.0) - 0.0
}

// Scalar const on a vector. `x:float3 * 1.0` folds to x (the non-const operand carries the
// result type). `x:float3 * 0.0` folds to a width-matched float3(0): zero_const_of keys on
// the RESULT type, so the scalar zero never mistypes float3 -> float. Both value-correct
// (finite inputs). The dedicated *vector-const* arms (`x * float3(1)`, …) are f_vc* below.
[flatten]
def f_vec1(x : float3) : float3 {
    return x * 1.0
}

[flatten]
def f_vec0(x : float3) : float3 {
    return x * 0.0
}

// ----- integer algebraic identities (two's-complement exact, incl. x*-1 at edges) -----

[flatten]
def f_imul1(x : int) : int {
    return x * 1
}

[flatten]
def f_imul1l(x : int) : int {
    return 1 * x
}

[flatten]
def f_imulneg1(x : int) : int {
    return x * -1
}

[flatten]
def f_imulneg1l(x : int) : int {
    return -1 * x
}

[flatten]
def f_imul0(x : int) : int {
    return x * 0
}

[flatten]
def f_iadd0(x : int) : int {
    return x + 0
}

[flatten]
def f_iadd0l(x : int) : int {
    return 0 + x
}

[flatten]
def f_isub0(x : int) : int {
    return x - 0
}

[flatten]
def f_izerosub(x : int) : int {
    return 0 - x
}

// Scalar const on an int vector: `x:int3 * 1` folds to x; `x:int3 * 0` folds to int3(0)
// (zero_const_of on the result type — exact for integers, no fast-math). The dedicated
// *vector-const* arms (`x * int3(1)`, …) are f_ivc* below.
[flatten]
def f_ivec1(x : int3) : int3 {
    return x * 1
}

[flatten]
def f_ivec0(x : int3) : int3 {
    return x * 0
}

// ----- const-constructor folding (#1c): vector ctors over const args fold to a
// single const literal (the typer leaves them; flatten collapses them) -----

[flatten]
def f_ctor_zero : float3 {
    return float3(0.0)
}

[flatten]
def f_ctor_arith : float3 {
    return float3(2.0 * 3.0)
}

[flatten]
def f_ctor_mixed : float3 {
    return float3(1.0, 2.0, 3.0)
}

// A const ctor buried under arithmetic with a runtime operand: the ctor folds, the
// surrounding add stays.
[flatten]
def f_ctor_mixed_rt(x : float3) : float3 {
    return x + float3(0.5)
}

// ----- vector-const operands: floatX / intX const operands fold like scalar consts -----
//
// Distinct from f_vec*/f_ivec* above (scalar const on a vector): here the CONST operand is
// itself a vector literal. `v * vec(1)` -> v, `v * vec(-1)` -> -v, `v + vec(0)` -> v,
// `vec(0) - v` -> -v, `v * vec(0)` -> vec(0). The residual walk (test_flatten_fold.das)
// proves the ungated arms fire (a survivor is flagged); the differential proves the value.

[flatten]
def f_vcmul1(x : float3) : float3 {
    return x * float3(1.0)
}

[flatten]
def f_vcmulneg1(x : float3) : float3 {
    return x * float3(-1.0)
}

[flatten]
def f_vcadd0(x : float3) : float3 {
    return x + float3(0.0)
}

[flatten]
def f_vcsub0(x : float3) : float3 {
    return x - float3(0.0)
}

[flatten]
def f_vczerosub(x : float3) : float3 {
    return float3(0.0) - x
}

[flatten]
def f_vcmul0(x : float3) : float3 {
    return x * float3(0.0)
}

// int vector-const operands — exact (no fast-math); covers IGRID incl. edges for `*-1`.
[flatten]
def f_ivcmul1(x : int3) : int3 {
    return x * int3(1)
}

[flatten]
def f_ivcmulneg1(x : int3) : int3 {
    return x * int3(-1)
}

[flatten]
def f_ivcadd0(x : int3) : int3 {
    return x + int3(0)
}

[flatten]
def f_ivcsub0(x : int3) : int3 {
    return x - int3(0)
}

[flatten]
def f_ivczerosub(x : int3) : int3 {
    return int3(0) - x
}

[flatten]
def f_ivcmul0(x : int3) : int3 {
    return x * int3(0)
}

// ----- uint: scalar identities (no *-1 — unsigned), vectors, and const constructors -----

[flatten]
def f_umul1(x : uint) : uint {
    return x * 1u
}

[flatten]
def f_uadd0(x : uint) : uint {
    return x + 0u
}

[flatten]
def f_usub0(x : uint) : uint {
    return x - 0u
}

[flatten]
def f_umul0(x : uint) : uint {
    return x * 0u
}

[flatten]
def f_uvmul1(x : uint3) : uint3 {
    return x * uint3(1u)
}

[flatten]
def f_uvadd0(x : uint3) : uint3 {
    return x + uint3(0u)
}

[flatten]
def f_uvmul0(x : uint3) : uint3 {
    return x * uint3(0u)
}

// uint const constructors fold to a literal (eval_to_const now covers uint scalar + vector).
[flatten]
def f_uctor : uint3 {
    return uint3(1u, 2u, 3u)
}

[flatten]
def f_uctor_splat : uint3 {
    return uint3(5u)
}

// ----- boolean algebraic identities (&&, ||, !, bool-select) -----
//
// `true && x` / `x || true` etc. carry a const operand the typer leaves alone (it
// folds only fully-const ops). In a no-early-return twin the lowering's mask-collapse
// pass folds them; the early-return variants below (f_*_early) reassign the live mask,
// so that pass bails and the post-infer FoldVisitor is the sole folder — exercising the
// new arms directly. Differential proves the fold is value-preserving either way.

[flatten]
def f_and_tl(x : bool) : bool {
    return true && x
}

[flatten]
def f_and_tr(x : bool) : bool {
    return x && true
}

[flatten]
def f_and_fl(x : bool) : bool {
    return false && x
}

[flatten]
def f_and_fr(x : bool) : bool {
    return x && false
}

[flatten]
def f_or_fl(x : bool) : bool {
    return false || x
}

[flatten]
def f_or_fr(x : bool) : bool {
    return x || false
}

[flatten]
def f_or_tl(x : bool) : bool {
    return true || x
}

[flatten]
def f_or_tr(x : bool) : bool {
    return x || true
}

[flatten]
def f_not_t : bool {
    return !true
}

[flatten]
def f_not_f : bool {
    return !false
}

[flatten]
def f_sel_tf(c : bool) : bool {
    return c ? true : false
}

[flatten]
def f_sel_ft(c : bool) : bool {
    return c ? false : true
}

// Early-return variants: the early return reassigns the live mask, so the mask-collapse
// pass bails (empty const map) and the post-infer FoldVisitor folds the boolean. Scalar
// return type auto-zero-inits, so no struct-init issue.
[flatten]
def f_and_early(x : bool; g : bool) : bool {
    if (g) {
        return true && x
    }
    return x || true
}

[flatten]
def f_sel_early(c : bool; g : bool) : bool {
    if (g) {
        return c ? true : false
    }
    return c ? false : true
}

// ----- const-propagation of single-def locals via SSA-rename (#2) -----
//
// A reassigned local is SSA-renamed to single-def versions at cycle 1, then the post-
// infer const-prop folds the chain. Differential proves the fold is value-preserving
// (an unfolded accumulator computes the same), incl. the soundness case where the
// accumulator depends on a parameter and must NOT fold. The cap_fold_accumulator shader
// proves it actually fires (0 add nodes reach the backend).

// Pure const accumulator chain -> a constant.
[flatten]
def f_acc_const : int {
    var acc = 5
    acc = acc + 3
    acc = acc * 2
    return acc
}

// Const loop accumulator (unrolled then folded): 0+0+1+2+3 = 6.
[flatten]
def f_acc_loop : int {
    var acc = 0
    for (i in range(4)) {
        acc = acc + i
    }
    return acc
}

// Const prefix folds, runtime tail stays: 30 + x.
[flatten]
def f_acc_mixed(x : int) : int {
    var acc = 1
    acc = acc + 2
    acc = acc * 10
    return acc + x
}

// Accumulator seeded from a parameter must NOT fold: (x + 5) * 2.
[flatten]
def f_acc_runtime(x : int) : int {
    var acc = x
    acc = acc + 5
    acc = acc * 2
    return acc
}

// Float const loop accumulator: 0.25 * 4 = 1.0.
[flatten]
def f_acc_float : float {
    var acc = 0.0
    for (_i in range(4)) {
        acc = acc + 0.25
    }
    return acc
}

// Const accumulator across an early return: g ? 10 : 15.
[flatten]
def f_acc_early(g : bool) : int {
    var acc = 10
    if (g) {
        return acc
    }
    acc = acc + 5
    return acc
}

// Guards the `__ssa_` rename-prefix choice: a user local literally named `flat_acc`
// (reassigned -> SSA-renamed) must stay a USER local — folded by const_prop_locals — not be
// mis-swept by the `__flat_*` mask/copy/dse passes. The chosen `__ssa_{orig}_{counter}`
// scheme renames it to `__ssa_flat_acc_N` (disjoint from `__flat_`); a naive `__{orig}`
// scheme would mint `__flat_acc_1` and is_flat_temp would misclassify it. Folds to 8.
[flatten]
def f_acc_flatname : int {
    var flat_acc = 5
    flat_acc = flat_acc + 3
    return flat_acc
}

// ----- const type-conversion folding -----
//
// A pure builtin (cast / vector ctor) with all-const args is a compile-time constant, so
// it folds regardless of the whitelist — without that, these leaf builtins (`float`,
// `int`, `float2`) are rejected as non-GPU-expressible and the functions would NOT
// COMPILE. So compiling at all proves the gate relaxed; the differential proves the value.

[flatten]
def f_cast_float : float {
    return float(7)
}

[flatten]
def f_cast_int : int {
    return int(3.9)
}

[flatten]
def f_cast_expr : float {
    return float(3) * 2.0 + float(1)
}

[flatten]
def f_cast_vec : float2 {
    return float2(float(1), float(2))
}

// The motivating case: a const cast inside a loop accumulator (was whitelist-rejected).
[flatten]
def f_cast_accum : float {
    var acc = 0.0
    for (i in range(4)) {
        acc += float(i) * 0.1
    }
    return acc
}

// ----- fold fixpoint across the typer boundary (#3) -----
//
// flatten's fold and the typer's const-fold are mutually-enabling, so the fold phase
// must iterate to a fixpoint — a single pass is not enough. flatten folds the operands
// the typer leaves (a const-propagated local `p`, a runtime-operand identity `0*b`);
// the re-infer THEN const-folds the freshly-constant `p >> (p & 31)` / `24 >> (24 & 31)`
// to `0` (a shift/mask the FoldVisitor does not touch), which exposes a NEW `x - 0`
// identity that only a SECOND fold pass can collapse. The residual walk (test_flatten_fold.das)
// flags a surviving `x - 0`, so a clean run proves the fixpoint loop holds; both reduce to
// `b + 7`.

// const-prop reveals the constant: `p → 24`, then the typer folds `24 >> (24 & 31) → 0`.
// `var` (not `let`) is load-bearing: an immutable `let p` is const-propagated early, so
// the typer folds the shift before fold_body runs and a single pass suffices. `var`
// defeats that early propagation, forcing the cascade into the post-fold re-infer — the
// path the fixpoint loop exists to handle.
[flatten]
def f_fix_local(b : int) : int {
    var p = 24   // nolint:LINT003 -- var defeats early const-prop; see note above
    return (b + 7) - (p >> (p & 31))
}

// inline-substitution reveals it via a runtime-operand identity flatten folds but the
// typer will not: `(0 * b) + 24 → 24`, then the typer folds the shift to `0`.
def fix_callee(p0, q : int) : int {
    return q - (p0 >> (p0 & 31))
}

[flatten]
def f_fix_inline(b : int) : int {
    return fix_callee((0 * b) + 24, b + 7)
}

// ----- reassociation (#1c): scattered consts gather into one folded literal -----
//
// `0.5 + x + 0.6` is NOT foldable by the typer (consts are non-adjacent; the typer folds only
// adjacent const arithmetic and never reassociates). flatten reassociates under fast-math, so the
// twin collapses to `x + 1.1` / `x * 6` / `x + 5` etc. Float arms use the relative-tolerance
// `fapprox` (reassoc genuinely changes rounding); integer arms are exact (`+`/`*` are associative).

[flatten]
def f_re_add(x : float) : float {
    return 0.5 + x + 0.6
}

[flatten]
def f_re_add2(x : float; y : float) : float {
    return 0.5 + x + y + 0.6
}

[flatten]
def f_re_sub(x : float) : float {
    return x - 0.3 - 0.2
}

[flatten]
def f_re_mul(x : float) : float {
    return 2.0 * x * 3.0
}

// Scalar consts broadcast over a vector var: `0.5 * 2.0 = 1.0`, then `v * 1.0 → v`. Reassoc must
// preserve the float3 result type through the gather (the `*1` cleanup carries its own
// same_base_type gate). Exact (1.0 is exact).
[flatten]
def f_re_broad(v : float3) : float3 {
    return v * 0.5 * 2.0
}

// Vector-const operands gather + fold via the all-const ExprOp2 arm (the typer leaves vector const
// arithmetic). `float3(0.5) + v + float3(0.6) → v + float3(1.1)`.
[flatten]
def f_re_vec(v : float3) : float3 {
    return float3(0.5) + v + float3(0.6)
}

[flatten]
def f_re_iadd(x : int) : int {
    return 2 + x + 3
}

[flatten]
def f_re_imul(x : int) : int {
    return 2 * x * 3
}

[flatten]
def f_re_isub(x : int) : int {
    return x - 3 - 2
}

// Var terms are sorted into a canonical (structural) order so commutative chains converge to one
// form (for a later CSE). Vars written out of order here (c, a, b) + scattered consts must give
// the same value as the original; the twin gathers to a sorted `((a + b) + c) + <const>`.
[flatten]
def f_re_sort(a : int; b : int; c : int) : int {
    return c + 3 + a + 1 + b
}

// Subtraction with out-of-order vars: sorting must carry the sign (`b - a` canonicalizes via
// `-a + b`), so the value is preserved.
[flatten]
def f_re_sortsub(a : int; b : int) : int {
    return b - a + 10 - 4
}

// ----- differential driver -----

var GRID : array<float>
var BOOLS : array<bool>
var IGRID : array<int>
var UGRID : array<uint>

[init]
def fill_grid {
    GRID <- [-100.0, -3.5, -1.0, -0.0, 0.0, 1.0, 2.5, 7.0, 100.0]
    BOOLS <- [true, false]
    IGRID <- [-100, -7, -1, 0, 1, 3, 42, 1000]
    UGRID <- [0u, 1u, 3u, 42u, 1000u, 4294967295u]
}

// Componentwise compares for the vector differential arms.
def cmp3f(t : T?; a, b : float3) {
    t |> equal(a.x, b.x); t |> equal(a.y, b.y); t |> equal(a.z, b.z)
}

def cmp3i(t : T?; a, b : int3) {
    t |> equal(a.x, b.x); t |> equal(a.y, b.y); t |> equal(a.z, b.z)
}

def cmp3u(t : T?; a, b : uint3) {
    t |> equal(a.x, b.x); t |> equal(a.y, b.y); t |> equal(a.z, b.z)
}

// Relative-tolerance float compare for the reassociation arms, where the regrouped twin rounds
// differently from the left-to-right original (the fast-math contract reassoc trades on).
def fapprox(t : T?; a, b : float) {
    let m = max(max(abs(a), abs(b)), 1.0)
    t |> success(abs(a - b) <= 1.0e-4 * m, "reassoc approx mismatch: {a} vs {b}")
}

def cmp3f_approx(t : T?; a, b : float3) {
    fapprox(t, a.x, b.x); fapprox(t, a.y, b.y); fapprox(t, a.z, b.z)
}

[test]
def test_flatten_const(t : T?) {
    t |> run("x * 1.0 -> x") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_mul1_flat(v), f_mul1(v))
        }
    }
    t |> run("1.0 * x -> x") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_mul1l_flat(v), f_mul1l(v))
        }
    }
    t |> run("x * -1.0 -> -x") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_mulneg1_flat(v), f_mulneg1(v))
        }
    }
    t |> run("x * 0.0 -> 0 (finite-exact)") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_mul0_flat(v), f_mul0(v))
        }
    }
    t |> run("x + 0.0 -> x") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_add0_flat(v), f_add0(v))
        }
    }
    t |> run("x - 0.0 -> x") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_sub0_flat(v), f_sub0(v))
        }
    }
    t |> run("0.0 - x -> -x") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_zerosub_flat(v), f_zerosub(v))
        }
    }
    t |> run("chained identities collapse") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_combo_flat(v), f_combo(v))
        }
    }
    t |> run("integer identities (*1, *-1, +0, -0, *0)") @(t : T?) {
        for (n in IGRID) {
            t |> equal(f_imul1_flat(n), f_imul1(n))
            t |> equal(f_imul1l_flat(n), f_imul1l(n))
            t |> equal(f_imulneg1_flat(n), f_imulneg1(n))
            t |> equal(f_imulneg1l_flat(n), f_imulneg1l(n))
            t |> equal(f_imul0_flat(n), f_imul0(n))
            t |> equal(f_iadd0_flat(n), f_iadd0(n))
            t |> equal(f_iadd0l_flat(n), f_iadd0l(n))
            t |> equal(f_isub0_flat(n), f_isub0(n))
            t |> equal(f_izerosub_flat(n), f_izerosub(n))
            t |> equal(f_imul1_flat(n), n)
            t |> equal(f_imul0_flat(n), 0)
        }
    }
    t |> run("float3 * 1.0 -> x (vector)") @(t : T?) {
        for (v in GRID) {
            let a = f_vec1(float3(v, v + 1.0, v - 1.0))
            let b = f_vec1_flat(float3(v, v + 1.0, v - 1.0))
            t |> equal(a.x, b.x)
            t |> equal(a.y, b.y)
            t |> equal(a.z, b.z)
        }
    }
    t |> run("float3 * 0.0 -> float3(0) (width-matched, no scalar mistype)") @(t : T?) {
        for (v in GRID) {
            let a = f_vec0(float3(v, v + 1.0, v - 1.0))
            let b = f_vec0_flat(float3(v, v + 1.0, v - 1.0))
            t |> equal(a.x, b.x)
            t |> equal(a.y, b.y)
            t |> equal(a.z, b.z)
        }
    }
    t |> run("int3 * 1 -> x (vector)") @(t : T?) {
        for (n in IGRID) {
            let a = f_ivec1(int3(n, n + 1, n - 1))
            let b = f_ivec1_flat(int3(n, n + 1, n - 1))
            t |> equal(a.x, b.x)
            t |> equal(a.y, b.y)
            t |> equal(a.z, b.z)
        }
    }
    t |> run("int3 * 0 -> int3(0) (width-matched, no scalar mistype)") @(t : T?) {
        for (n in IGRID) {
            let a = f_ivec0(int3(n, n + 1, n - 1))
            let b = f_ivec0_flat(int3(n, n + 1, n - 1))
            t |> equal(a.x, b.x)
            t |> equal(a.y, b.y)
            t |> equal(a.z, b.z)
        }
    }
    t |> run("const ctor float3(0.0) folds to a const") @(t : T?) {
        let a = f_ctor_zero()
        let b = f_ctor_zero_flat()
        t |> equal(a.x, b.x); t |> equal(a.y, b.y); t |> equal(a.z, b.z)
    }
    t |> run("const ctor float3(2*3) folds to float3(6)") @(t : T?) {
        let a = f_ctor_arith()
        let b = f_ctor_arith_flat()
        t |> equal(a.x, b.x); t |> equal(b.x, 6.0)
    }
    t |> run("const ctor float3(1,2,3) folds") @(t : T?) {
        let a = f_ctor_mixed()
        let b = f_ctor_mixed_flat()
        t |> equal(a.x, b.x); t |> equal(a.y, b.y); t |> equal(a.z, b.z)
    }
    t |> run("const ctor under runtime add") @(t : T?) {
        for (v in GRID) {
            let a = f_ctor_mixed_rt(float3(v, v + 1.0, v - 1.0))
            let b = f_ctor_mixed_rt_flat(float3(v, v + 1.0, v - 1.0))
            t |> equal(a.x, b.x); t |> equal(a.y, b.y); t |> equal(a.z, b.z)
        }
    }
    t |> run("float3 vector-const operands (* / + / - vs float3 literals)") @(t : T?) {
        for (v in GRID) {
            let inp = float3(v, v + 1.0, v - 1.0)
            cmp3f(t, f_vcmul1_flat(inp), f_vcmul1(inp))
            cmp3f(t, f_vcmulneg1_flat(inp), f_vcmulneg1(inp))
            cmp3f(t, f_vcadd0_flat(inp), f_vcadd0(inp))
            cmp3f(t, f_vcsub0_flat(inp), f_vcsub0(inp))
            cmp3f(t, f_vczerosub_flat(inp), f_vczerosub(inp))
            cmp3f(t, f_vcmul0_flat(inp), f_vcmul0(inp))
        }
    }
    t |> run("int3 vector-const operands (exact)") @(t : T?) {
        for (n in IGRID) {
            let inp = int3(n, n + 1, n - 1)
            cmp3i(t, f_ivcmul1_flat(inp), f_ivcmul1(inp))
            cmp3i(t, f_ivcmulneg1_flat(inp), f_ivcmulneg1(inp))
            cmp3i(t, f_ivcadd0_flat(inp), f_ivcadd0(inp))
            cmp3i(t, f_ivcsub0_flat(inp), f_ivcsub0(inp))
            cmp3i(t, f_ivczerosub_flat(inp), f_ivczerosub(inp))
            cmp3i(t, f_ivcmul0_flat(inp), f_ivcmul0(inp))
        }
    }
    t |> run("uint scalar identities (*1, +0, -0, *0)") @(t : T?) {
        for (u in UGRID) {
            t |> equal(f_umul1_flat(u), f_umul1(u))
            t |> equal(f_uadd0_flat(u), f_uadd0(u))
            t |> equal(f_usub0_flat(u), f_usub0(u))
            t |> equal(f_umul0_flat(u), f_umul0(u))
            t |> equal(f_umul1_flat(u), u)
            t |> equal(f_umul0_flat(u), 0u)
        }
    }
    t |> run("uint3 vector-const operands (*1, +0, *0)") @(t : T?) {
        for (u in UGRID) {
            let inp = uint3(u, u + 1u, u + 2u)
            cmp3u(t, f_uvmul1_flat(inp), f_uvmul1(inp))
            cmp3u(t, f_uvadd0_flat(inp), f_uvadd0(inp))
            cmp3u(t, f_uvmul0_flat(inp), f_uvmul0(inp))
        }
    }
    t |> run("uint const constructors fold (eval_to_const)") @(t : T?) {
        cmp3u(t, f_uctor_flat(), f_uctor())
        cmp3u(t, f_uctor_splat_flat(), f_uctor_splat())
        let c = f_uctor_flat()
        t |> equal(c.x, 1u); t |> equal(c.y, 2u); t |> equal(c.z, 3u)
    }
    t |> run("__ssa_ rename keeps a flat_-named user local safe") @(t : T?) {
        t |> equal(f_acc_flatname_flat(), f_acc_flatname())
        t |> equal(f_acc_flatname_flat(), 8)
    }
    t |> run("true && x -> x ; x && true -> x") @(t : T?) {
        for (b in BOOLS) {
            t |> equal(f_and_tl_flat(b), f_and_tl(b))
            t |> equal(f_and_tr_flat(b), f_and_tr(b))
            t |> equal(f_and_tl_flat(b), b)
        }
    }
    t |> run("false && x -> false ; x && false -> false") @(t : T?) {
        for (b in BOOLS) {
            t |> equal(f_and_fl_flat(b), f_and_fl(b))
            t |> equal(f_and_fr_flat(b), f_and_fr(b))
            t |> equal(f_and_fl_flat(b), false)
        }
    }
    t |> run("false || x -> x ; x || false -> x") @(t : T?) {
        for (b in BOOLS) {
            t |> equal(f_or_fl_flat(b), f_or_fl(b))
            t |> equal(f_or_fr_flat(b), f_or_fr(b))
            t |> equal(f_or_fl_flat(b), b)
        }
    }
    t |> run("true || x -> true ; x || true -> true") @(t : T?) {
        for (b in BOOLS) {
            t |> equal(f_or_tl_flat(b), f_or_tl(b))
            t |> equal(f_or_tr_flat(b), f_or_tr(b))
            t |> equal(f_or_tl_flat(b), true)
        }
    }
    t |> run("!true -> false ; !false -> true") @(t : T?) {
        t |> equal(f_not_t_flat(), f_not_t())
        t |> equal(f_not_f_flat(), f_not_f())
        t |> equal(f_not_t_flat(), false)
        t |> equal(f_not_f_flat(), true)
    }
    t |> run("c ? true : false -> c ; c ? false : true -> !c") @(t : T?) {
        for (b in BOOLS) {
            t |> equal(f_sel_tf_flat(b), b)
            t |> equal(f_sel_ft_flat(b), !b)
        }
    }
    t |> run("boolean fold under early return (FoldVisitor path)") @(t : T?) {
        for (g in BOOLS) {
            for (b in BOOLS) {
                t |> equal(f_and_early_flat(b, g), f_and_early(b, g))
                t |> equal(f_sel_early_flat(b, g), f_sel_early(b, g))
            }
        }
    }
    t |> run("const accumulator chain -> constant") @(t : T?) {
        t |> equal(f_acc_const_flat(), f_acc_const())
        t |> equal(f_acc_const_flat(), 16)
    }
    t |> run("const loop accumulator -> constant") @(t : T?) {
        t |> equal(f_acc_loop_flat(), f_acc_loop())
        t |> equal(f_acc_loop_flat(), 6)
    }
    t |> run("const prefix folds, runtime tail stays") @(t : T?) {
        for (x in IGRID) {
            t |> equal(f_acc_mixed_flat(x), f_acc_mixed(x))
            t |> equal(f_acc_mixed_flat(x), 30 + x)
        }
    }
    t |> run("param-seeded accumulator must NOT fold (soundness)") @(t : T?) {
        for (x in IGRID) {
            t |> equal(f_acc_runtime_flat(x), f_acc_runtime(x))
            t |> equal(f_acc_runtime_flat(x), (x + 5) * 2)
        }
    }
    t |> run("float const loop accumulator -> 1.0") @(t : T?) {
        t |> equal(f_acc_float_flat(), f_acc_float())
        t |> equal(f_acc_float_flat(), 1.0)
    }
    t |> run("const accumulator across early return") @(t : T?) {
        for (g in BOOLS) {
            t |> equal(f_acc_early_flat(g), f_acc_early(g))
        }
    }
    t |> run("const cast float(7) / int(3.9) fold") @(t : T?) {
        t |> equal(f_cast_float_flat(), f_cast_float())
        t |> equal(f_cast_float_flat(), 7.0)
        t |> equal(f_cast_int_flat(), f_cast_int())
        t |> equal(f_cast_int_flat(), 3)
    }
    t |> run("const casts inside an expression fold") @(t : T?) {
        t |> equal(f_cast_expr_flat(), f_cast_expr())
        t |> equal(f_cast_expr_flat(), 7.0)
    }
    t |> run("const vector ctor over casts folds") @(t : T?) {
        let a = f_cast_vec_flat()
        t |> equal(a.x, 1.0)
        t |> equal(a.y, 2.0)
    }
    t |> run("const cast in accumulator folds (was whitelist-rejected)") @(t : T?) {
        t |> equal(f_cast_accum_flat(), f_cast_accum())
    }
    t |> run("fold reaches a fixpoint across the typer boundary -> b + 7") @(t : T?) {
        for (n in IGRID) {
            t |> equal(f_fix_local_flat(n), f_fix_local(n))
            t |> equal(f_fix_local_flat(n), n + 7)
            t |> equal(f_fix_inline_flat(n), f_fix_inline(n))
            t |> equal(f_fix_inline_flat(n), n + 7)
        }
    }
    t |> run("reassoc: scattered float consts gather (approx — rounding differs)") @(t : T?) {
        for (v in GRID) {
            fapprox(t, f_re_add_flat(v), f_re_add(v))
            fapprox(t, f_re_sub_flat(v), f_re_sub(v))
            fapprox(t, f_re_mul_flat(v), f_re_mul(v))
            for (w in GRID) {
                fapprox(t, f_re_add2_flat(v, w), f_re_add2(v, w))
            }
        }
    }
    t |> run("reassoc: scalar broadcast (v*0.5*2.0 -> v) + vector-const gather") @(t : T?) {
        for (v in GRID) {
            let vv = float3(v, v * 2.0, -v)
            cmp3f(t, f_re_broad_flat(vv), f_re_broad(vv))
            cmp3f_approx(t, f_re_vec_flat(vv), f_re_vec(vv))
        }
    }
    t |> run("reassoc: integer chains are exact") @(t : T?) {
        for (n in IGRID) {
            t |> equal(f_re_iadd_flat(n), f_re_iadd(n))
            t |> equal(f_re_iadd_flat(n), n + 5)
            t |> equal(f_re_imul_flat(n), f_re_imul(n))
            t |> equal(f_re_imul_flat(n), n * 6)
            t |> equal(f_re_isub_flat(n), f_re_isub(n))
            t |> equal(f_re_isub_flat(n), n - 5)
        }
    }
    t |> run("reassoc: out-of-order vars are sorted, value preserved") @(t : T?) {
        for (a in IGRID) {
            for (b in IGRID) {
                t |> equal(f_re_sort_flat(a, b, 0), f_re_sort(a, b, 0))
                t |> equal(f_re_sortsub_flat(a, b), f_re_sortsub(a, b))
                t |> equal(f_re_sortsub_flat(a, b), b - a + 6)
            }
        }
    }
}
