options gen2
options indenting = 4
options fast_math = true

require dastest/testing_boost
require math

// fast_math must stay VALUE-SAFE for real infs: a sigmoid's exp(-v) overflows float for
// v < -88, and the old all-flags stamp (nnan|ninf|arcp|...) let the x64 JIT lower the
// division via rcp-estimate + Newton, whose refinement computes inf*0 = NaN — the whole
// parakeet encoder went NaN (aarch64 never took that lowering). The stamp is now
// reassoc|nsz|contract only (llvm_jit_run.das apply_fast_math_to_module).

// bit-pattern NaN check: a float `v != v` itself folds to false under an nnan stamp,
// so the assert must not trust FP compares
def is_nan_bits(v : float) : bool {
    let bits = unsafe(reinterpret<uint>(v))
    return (bits & 0x7F800000u) == 0x7F800000u && (bits & 0x007FFFFFu) != 0u
}

// the engine silu's exact pointer-core shape (dasllama_math.das)
def silu_core(var x : float?; size : int64) {
    unsafe {
        for (i in range64(size)) {
            let v = x[i]
            x[i] = v / (1.0 + exp(-v))
        }
    }
}

[test]
def test_fast_math_inf_safe(t : T?) {
    t |> run("silu stays finite across the exp-overflow range under fast_math") @(t : T?) {
        var x : array<float>
        for (i in range(4096)) {
            x |> push(float(i % 512 - 256) * 0.75)   // sweeps -192..191 — exp(-v) overflows past |v| ~ 88
        }
        silu_core(unsafe(addr(x[0])), int64(length(x)))
        var nans = 0
        for (v in x) {
            if (is_nan_bits(v)) {
                nans++
            }
        }
        t |> equal(nans, 0, "no NaNs from silu over the full range")
        // spot values stay INSIDE |v| < 88: interp exp is vecmath v_exp (sim_policy.h), which
        // does NOT saturate on overflow — NEON exp(192) wraps the exponent into the sign bit
        // (-0) instead of +inf, so silu values past the overflow knee are tier/platform
        // dependent. The NaN sweep above is the actual x64 rcp-Newton regression pin.
        t |> success(abs(x[168]) < 1e-6, "large-negative silu is ~0")          // silu(-66)
        t |> success(abs(x[344] - 66.0) < 1e-2, "large-positive silu is ~identity")   // silu(66)
    }
}
