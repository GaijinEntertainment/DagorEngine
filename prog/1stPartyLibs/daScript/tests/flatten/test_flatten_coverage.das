options gen2
options indenting = 4
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/flatten

//! Branch-coverage differential tests for daslib/flatten. Each function targets
//! a specific lowering/lifting branch NOT exercised by test_flatten_basic.das;
//! every twin is run against its original over an input grid AND (via the
//! macro's own cycle-2 scan) proven branchless + call-free at compile time.

// ----- helper callees -----

def sq(a : int) : int {
    return a * a
}

def addk(a : int; k : int) : int {
    return a + k
}

def konst : int {       // 0-parameter callee
    return 42
}

// ----- lift_expr: ExprOp1 (unary) wrapping a user call -----
[flatten]
def f_unary(x : int) : int {
    return -sq(x) + 1
}

// ----- lift_expr: ExprOp3 (source ternary) with a user call in each arm -----
// Branchless lowering inlines BOTH arms unconditionally, then selects — relies
// on both callees being pure (double-eval is harmless).
[flatten]
def f_ternary(x : int) : int {
    return x > 0 ? sq(x) : addk(x, 5)
}

// ----- inline_call: structPred non-null (user call inside an if branch) -----
[flatten]
def f_call_in_if(x : int) : int {
    if (x > 0) {
        return sq(x)
    }
    return x
}

// ----- inline_call: 0-parameter callee (param-bind loop runs zero times) -----
[flatten]
def f_zero_param(x : int) : int {
    return x + konst()
}

// ----- lower_stmt: bare expression statement (discarded user call) -----
[flatten]
def f_discard(x : int) : int {
    sq(x)
    return x + 1
}

// ----- lower_stmt: bare block (lexical scope) -----
[flatten]
def f_block(x : int) : int {
    var r = x
    {
        r = r + 1
    }
    return r
}

// ----- lower_stmt: bare block declaring a block-local (scope is omittable) -----
[flatten]
def f_block_local(x : int) : int {
    var r = x
    {
        let y = r * 3
        r = y + 1
    }
    return r
}

// ----- lower_stmt: `with` unwrap (pure infer-time scope; node simulates to body) -----
struct Q {
    px : int
    py : int
}

[flatten]
def f_with(n : int) : int {
    let q = Q(px = n, py = n * 2)
    var acc = n
    with (q) {
        acc += px + py     // px, py resolve to q.px, q.py at infer; with is omittable
    }
    return acc
}

// ----- lower_stmt: ExprLet, multiple variables + no initializer (v.init==null) -----
[flatten]
def f_multivar(x : int) : int {
    var a, b : int   // no-init is the point: covers the v.init==null branch
    a = x   // nolint:STYLE011
    b = x + 1
    return a + b
}

// ----- void entry: early return + predicated writes to an output global -----
// Mirrors the shader pattern where a void shader writes outputs to globals.
var g_vout : int

[flatten]
def vshader(x : int) {
    if (x < 0) {
        g_vout = -1
        return
    }
    g_vout = x * 10
}

// ----- void callee: inlined for its side effect (global write) only -----
var g_sink : int

def veffect(x : int) {
    g_sink = x * 2
}

[flatten]
def usev(x : int) : int {
    veffect(x)
    return x + 1
}

// ----- driver -----

var GRID : array<int>

[init]
def fill_grid {
    GRID <- [-201, -101, -7, -1, 0, 1, 7, 100, 101, 250]
}

[test]
def test_flatten_coverage(t : T?) {
    t |> run("ExprOp1 unary wrapping call") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_unary_flat(v), f_unary(v))
        }
    }
    t |> run("ExprOp3 source ternary, call per arm") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_ternary_flat(v), f_ternary(v))
        }
    }
    t |> run("call inside if branch (structPred non-null)") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_call_in_if_flat(v), f_call_in_if(v))
        }
    }
    t |> run("zero-parameter callee") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_zero_param_flat(v), f_zero_param(v))
        }
    }
    t |> run("bare discarded call statement") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_discard_flat(v), f_discard(v))
        }
    }
    t |> run("bare block scope") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_block_flat(v), f_block(v))
        }
    }
    t |> run("bare block declaring a block-local") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_block_local_flat(v), f_block_local(v))
        }
    }
    t |> run("with-scope unwrap") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_with_flat(v), f_with(v))
        }
    }
    t |> run("let with multiple variables") @(t : T?) {
        for (v in GRID) {
            t |> equal(f_multivar_flat(v), f_multivar(v))
        }
    }
    t |> run("void entry: predicated write to output global") @(t : T?) {
        for (v in GRID) {
            g_vout = 0
            vshader(v)
            let orig = g_vout
            g_vout = 0
            vshader_flat(v)
            t |> equal(g_vout, orig)
        }
    }
    t |> run("void callee inlined for its global-write side effect") @(t : T?) {
        for (v in GRID) {
            g_sink = 0
            let origRet = usev(v)
            let origSink = g_sink
            g_sink = 0
            let flatRet = usev_flat(v)
            t |> equal(flatRet, origRet)
            t |> equal(g_sink, origSink)
        }
    }
}
