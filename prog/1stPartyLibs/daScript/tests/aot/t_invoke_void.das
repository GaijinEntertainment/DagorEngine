options gen2
options indenting = 4
options stack = 4096
options no_coverage

require dastest/testing_boost public
require jit

// Test that das_invoke_function<void> uses AOT dispatch (direct call)
// rather than callOrFastcall (which pushes daScript stack frames).
//
// Without the fix, each invoke(@@void_recurse, n) goes through callOrFastcall,
// pushing a stack frame. With a small stack (4096) and deep recursion, this overflows.
// With the fix, invoke uses aotFunction directly — no daScript stack frames — so it works.
//
// Tier map: AOT → aotFunction dispatch, frame-less, succeeds. JIT with the invoke
// fastpath armed (has_jit_fastpath) → jitFunction dispatch, frame-less too, succeeds.
// Pure interpreter (and a JIT that declined these functions) → callOrFastcall pushes
// frames and overflows — that's expected.

var g_counter : int = 0

var g_void_fn : function<(n : int) : void>

[hybrid]
def void_recurse(n : int) {
    g_counter ++
    if (n > 0) {
        invoke(g_void_fn, n - 1)
    }
}

var g_int_fn : function<(n : int) : int>

[hybrid]
def int_recurse(n : int) : int {
    g_counter ++
    if (n > 0) {
        return invoke(g_int_fn, n - 1)
    }
    return g_counter
}

[test]
def test_invoke_void_aot_dispatch(t : T?) {
    let depth = 150
    let use_aot = get_command_line_arguments() |> has_value("--use-aot")
    t |> run("void invoke uses AOT dispatch (no stack frames)") @(t : T?) {
        g_void_fn = @@void_recurse
        g_counter = 0
        var ok = true
        try {
            invoke(g_void_fn, depth)
        } recover {
            ok = false
        }
        if (use_aot || has_jit_fastpath(g_void_fn)) {
            // AOT (aotFunction) or armed JIT fastpath (jitFunction): frame-less — should succeed
            t |> success(ok)
            if (ok) {
                t |> equal(g_counter, depth + 1)
            }
        } else {
            // In interpreter: invoke uses callOrFastcall, pushes stack frames — should overflow
            t |> success(!ok)
        }
    }
    t |> run("non-void invoke uses AOT dispatch (baseline)") @(t : T?) {
        g_int_fn = @@int_recurse
        g_counter = 0
        var ok = true
        try {
            invoke(g_int_fn, depth)
        } recover {
            ok = false
        }
        if (use_aot || has_jit_fastpath(g_int_fn)) {
            // AOT (aotFunction) or armed JIT fastpath (jitFunction): frame-less — should succeed
            t |> success(ok)
        } else {
            // In interpreter: should overflow
            t |> success(!ok)
        }
    }
}
