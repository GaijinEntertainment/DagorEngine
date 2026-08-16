options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

// Regression for issue #3321: a callee that writes its output pointer ONLY through a
// helper call, passing the pointer OFFSET (`cpy(d + k*2, ...)`) inside a loop, must still
// be inferred [modify_argument]. The offset desugars to `i_das_ptr_add(d, ...)` — a
// firstArgReturnType builtin whose result aliases the base pointer's pointee. Before the
// fix the side-effect analyzer did not recurse through that builtin, so the forwarded write
// never reached `d`, `pk` collapsed to [nosideeffects], and the whole call was
// dead-code-eliminated — silently dropping the write in interp, JIT and AOT alike.

// leaf helper: writes every element of its (mutable) output pointer
def cpy2(var d : float?; s : float const?; n : int64) {
    for (j in range64(n)) { unsafe { d[j] = s[j] + 100.0 } }
}

// the #3321 surface: writes d ONLY via the helper, passing d offset by the loop var
def pk(var d : float?; s : float const?) {
    unsafe { for (k in range64(2l)) { cpy2(d + k * 2l, s + k * 2l, 2l) } }
}

// control: writes d directly (no helper) — the write was never lost
def pk_direct(var d : float?; s : float const?) {
    unsafe { for (k in range64(4l)) { d[k] = s[k] + 100.0 } }
}

// control: helper called with the pointer UNOFFSET — plain ExprVar arg, always propagated
def pk_unoffset(var d : float?; s : float const?) {
    unsafe { cpy2(d, s, 4l) }
}

def fill(var a : float[4]; base : float) {
    for (i in range(4)) { a[i] = base + float(i) }
}

[test]
def test_offset_pointer_write_through_helper(t : T?) {
    // (1) the bug: offset-pointer helper call in a loop — a stayed -9 before the fix
    t |> run("offset-pointer helper write") @(t : T?) {
        var a : float[4]; fill(a, -9.0)
        var b : float[4]; fill(b, 0.0)
        unsafe { pk(addr(a[0]), addr(b[0])) }
        t |> equal(a[0], 100.0)
        t |> equal(a[1], 101.0)
        t |> equal(a[2], 102.0)
        t |> equal(a[3], 103.0)
    }
    // (2) control: direct write, no helper — always worked
    t |> run("direct write control") @(t : T?) {
        var a : float[4]; fill(a, -9.0)
        var b : float[4]; fill(b, 0.0)
        unsafe { pk_direct(addr(a[0]), addr(b[0])) }
        t |> equal(a[0], 100.0)
        t |> equal(a[3], 103.0)
    }
    // (3) control: unoffset helper call — plain ExprVar argument, always worked
    t |> run("unoffset helper control") @(t : T?) {
        var a : float[4]; fill(a, -9.0)
        var b : float[4]; fill(b, 0.0)
        unsafe { pk_unoffset(addr(a[0]), addr(b[0])) }
        t |> equal(a[0], 100.0)
        t |> equal(a[3], 103.0)
    }
}
