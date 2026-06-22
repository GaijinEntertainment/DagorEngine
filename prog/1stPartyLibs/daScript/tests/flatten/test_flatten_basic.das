options gen2
options indenting = 4
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/flatten
require math

//! Differential tests for daslib/flatten: each `[flatten]`-annotated function
//! is run against its generated `<name>_flat` twin over an input grid; the
//! flattened (branchless, call-free) twin must compute identical results.
//!
//! NOTE: the interpreter's `?:` short-circuits, so these tests prove logical
//! EQUIVALENCE — they do not exercise the branchless "both arms execute"
//! behavior of a real GPU backend. See tests/flatten/README.md.

// ----- functions under flatten -----

[flatten]
def tail(x : int) : int {
    return x * 2 + 1
}

[flatten]
def guard(x : int) : int {
    if (x < 0) {
        return -1
    }
    return x * 10
}

[flatten]
def nested(x : int) : int {
    var r = 0
    if (x < 0) {
        r = -1
    } else {
        if (x > 100) {
            return 999
        }
        r = x
    }
    return r + 1
}

def square(a : int) : int {
    return a * a
}

def addk(a : int; k : int) : int {
    return a + k
}

[flatten]
def inlines(x : int) : int {
    let s = square(x)
    return addk(s, 3) + square(x + 1)
}

def cc(n : int) : int {
    return n + 100
}

def bb(n : int) : int {
    return cc(n) * 2
}

def aa(n : int) : int {
    return bb(n) - 1
}

[flatten]
def transitive(x : int) : int {
    return aa(x) + aa(x + 1)
}

[flatten]
def with_builtin(x : int) : int {
    return max(x, 0) + min(x, 5)
}

// helper WITH a local — inlining it at multiple sites (loop unroll, or two direct
// call sites) must uniquify the callee's locals, else the copies emit a colliding
// `var t` in the single flattened scope (error[30704]). Every other helper here is
// a single-expression return with no locals, so this case was previously uncovered.
def withlocal(a : int) : int {
    let t = a * 3
    let u = t + 1
    return u * t
}

[flatten]
def inline_loop(x : int) : int {
    var acc = 0
    for (i in range(3)) {
        acc += withlocal(x + i)
    }
    return acc
}

[flatten]
def inline_twice(x : int) : int {
    return withlocal(x) + withlocal(x + 1)
}

// A STRUCT return type with an early return: the predicated `__flat_ret = pred ? v :
// __flat_ret` write reads the bare accumulator decl, which a live early return leaves
// intact (the mask-fold can only collapse it for a single unconditional return). For a
// struct (unsafe-when-uninitialized) result that read was previously rejected
// (error[31016]); the lowering now marks the accumulator safe-when-uninitialized because
// the placeholder read is always discarded.
struct Pt {
    a : float3 = float3(0)
    b : float = 1.0
}

[flatten]
def struct_uncond(x : float) : Pt {
    return Pt(a = float3(x))
}

[flatten]
def struct_guard(x : float; c : bool) : Pt {
    if (c) {
        return Pt(a = float3(x))
    }
    return Pt(b = x)
}

// ----- partially-assigned bare decls: each branch writes a DIFFERENT var; the other keeps its
// implicit zero (the lowering materializes the zero-init the predicated select's false path reads) -----

[flatten]
def partial_vec(x : float) : float3 {
    var a : float3
    var b : float3
    if (x > 0.5) {
        a = float3(1.0, 0.0, 0.0)
    } else {
        b = float3(0.0, 1.0, 0.0)
    }
    return a + b
}

[flatten]
def partial_scalar(x : float) : float {
    var s : float
    if (x > 0.5) {
        s = x * 2.0
    }
    return s + 1.0
}

// ----- differential driver -----

var GRID : array<int>

[init]
def fill_grid {
    GRID <- [-201, -101, -7, -1, 0, 1, 7, 100, 101, 250]
}

[test]
def test_flatten_equivalence(t : T?) {
    t |> run("tail return") @(t : T?) {
        for (v in GRID) {
            t |> equal(tail_flat(v), tail(v))
        }
    }
    t |> run("early-return guard") @(t : T?) {
        for (v in GRID) {
            t |> equal(guard_flat(v), guard(v))
        }
    }
    t |> run("nested if/else + early return + local mutation") @(t : T?) {
        for (v in GRID) {
            t |> equal(nested_flat(v), nested(v))
        }
    }
    t |> run("inlining (let-init + nested-expression calls)") @(t : T?) {
        for (v in GRID) {
            t |> equal(inlines_flat(v), inlines(v))
        }
    }
    t |> run("transitive inlining a->b->c, lifted operands") @(t : T?) {
        for (v in GRID) {
            t |> equal(transitive_flat(v), transitive(v))
        }
    }
    t |> run("whitelisted builtins") @(t : T?) {
        for (v in GRID) {
            t |> equal(with_builtin_flat(v), with_builtin(v))
        }
    }
    t |> run("helper-with-local inlined across an unrolled loop") @(t : T?) {
        for (v in GRID) {
            t |> equal(inline_loop_flat(v), inline_loop(v))
        }
    }
    t |> run("helper-with-local inlined at two direct call sites") @(t : T?) {
        for (v in GRID) {
            t |> equal(inline_twice_flat(v), inline_twice(v))
        }
    }
    t |> run("partially-assigned bare decls (each branch writes a different var)") @(t : T?) {
        for (v in GRID) {
            let fv = float(v) * 0.1
            let pv = partial_vec_flat(fv)
            let ev = partial_vec(fv)
            t |> equal(pv.x, ev.x)
            t |> equal(pv.y, ev.y)
            t |> equal(pv.z, ev.z)
            t |> equal(partial_scalar_flat(fv), partial_scalar(fv))
        }
    }
    t |> run("struct return type, unconditional + early return (was error[31016])") @(t : T?) {
        for (v in GRID) {
            let fv = float(v)
            let u = struct_uncond_flat(fv)
            let eu = struct_uncond(fv)
            t |> equal(u.a.x, eu.a.x)
            t |> equal(u.b, eu.b)
            for (c in fixed_array(false, true)) {
                let g = struct_guard_flat(fv, c)
                let eg = struct_guard(fv, c)
                t |> equal(g.a.x, eg.a.x)
                t |> equal(g.b, eg.b)
            }
        }
    }
}
