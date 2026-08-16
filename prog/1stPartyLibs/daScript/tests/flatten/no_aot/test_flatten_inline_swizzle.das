options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/flatten

//! Regression for the helper-inline swizzle bug. flatten inlines a helper that reads a by-value
//! VECTOR parameter through a single-component swizzle (`p.x`). copy_prop then substitutes the call
//! argument into the swizzle base. That argument is a VALUE — arithmetic, or the value-copy of a
//! ref — so the swizzle becomes non-ref, while the `p.x`-of-a-ref-local read it replaced still
//! carried an ExprRef2Value. Before the typer folded ref2value-of-a-value (ast_infer_type.cpp), the
//! generated `_flat` twin failed to compile with error[30921] "can only dereference a reference".
//! Reported as the shaders heal_3d / force_field (examples/flatten); these cover the same shapes in
//! plain flatten. The file COMPILING proves the twins build; the equal() checks prove equivalence.

// helper whose by-value vector param is read via single-component swizzles
def axis_mix(p : float3) : float {
    return p.x * 0.5 + p.y - p.z * 0.25
}

// value source: the call argument is arithmetic (force_field's `(scrolledPos * gridScale).x`)
[flatten]
def inl_swizzle_val(s : float) : float {
    return axis_mix(float3(s, s * 2.0, s - 1.0) * 1.5)
}

// ref source: the argument is a (by-value) vector param; the `var __flat_arg = v` value-copy
// still feeds a non-ref swizzle base after copy_prop (heal_3d's `inp.worldPos`)
[flatten]
def inl_swizzle_ref(v : float3) : float {
    return axis_mix(v)
}

// loop-unroll + helper inline, accumulating swizzled reads (heal_3d's per-seed loop)
def hatch(p : float3; k : float) : float {
    return p.y * k + p.x
}

[flatten]
def inl_swizzle_loop(base : float3) : float {
    var acc = 0.0
    for (i in range(3)) {
        acc += hatch(base * float(i + 1), float(i) * 0.5)
    }
    return acc
}

var GRID : array<float>

[init]
def fill_grid {
    GRID <- [-7.0, -1.0, 0.0, 1.0, 2.5, 7.0]
}

[test]
def test_flatten_inline_swizzle(t : T?) {
    t |> run("helper swizzle of a value-source vector arg") @(t : T?) {
        for (v in GRID) {
            t |> equal(inl_swizzle_val_flat(v), inl_swizzle_val(v))
        }
    }
    t |> run("helper swizzle of a ref-source vector arg") @(t : T?) {
        for (v in GRID) {
            let p = float3(v, -v, v * 0.5)
            t |> equal(inl_swizzle_ref_flat(p), inl_swizzle_ref(p))
        }
    }
    t |> run("helper swizzle inlined across an unrolled loop") @(t : T?) {
        for (v in GRID) {
            let p = float3(v, -v, v * 0.5)
            t |> equal(inl_swizzle_loop_flat(p), inl_swizzle_loop(p))
        }
    }
}
