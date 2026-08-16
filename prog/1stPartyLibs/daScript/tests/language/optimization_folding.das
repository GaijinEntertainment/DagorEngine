options gen2

require dastest/testing_boost public
require daslib/defer

// behavioral coverage for the ConstFolding algebraic identities: every fold must be
// value-preserving, and purity gates must keep impure operands evaluated. folding
// QUALITY (that the rewrite actually fired) is asserted by hand via `options log`.

var g_i = 7
var g_u = 0xDEADu
var g_i64 = 9000000000l
var g_f = 3.5
var g_d = 2.25lf
var g_v = float3(1.0, 2.0, 4.0)
var g_b = true
var g_calls = 0

def side_i : int {
    g_calls++
    return g_i
}

def side_b : bool {
    g_calls++
    return g_b
}

def only_finally {
    defer() {
        g_calls++
    }
}

def void_return_impure {
    if (side_b()) { // nolint:LINT009 -- deliberate: equivalent void arms are the fold under test
        return
    } else {
        return
    }
}

def elif_grade(x : int) : int {
    if (x > 10) {
        return 3
    } elif (x > 5) {
        return 2
    } elif (x > 0) {
        return 1
    } else {
        return 0
    }
}

def defer_in_arm(c : bool) : int {
    if (c) {
        defer() {
            g_calls++
        }
        return 1
    } else {
        return 2
    }
}

[test]
def test_mul_identities(t : T?) {
    t |> run("x*1 and 1*x") @(t : T?) {
        t |> equal(g_i * 1, 7)
        t |> equal(1 * g_i, 7)
        t |> equal(g_f * 1.0, 3.5)
        t |> equal(1.0 * g_f, 3.5)
        t |> equal(g_d * 1.0lf, 2.25lf)
        t |> equal(g_v * 1.0, float3(1.0, 2.0, 4.0))
    }
    t |> run("x*-1") @(t : T?) {
        t |> equal(g_i * -1, -7)
        t |> equal(g_f * -1.0, -3.5)
    }
    t |> run("x*0 folds for int, keeps impure operands") @(t : T?) {
        t |> equal(g_i * 0, 0)
        t |> equal(0 * g_i, 0)
        g_calls = 0
        t |> equal(side_i() * 0, 0)
        t |> equal(g_calls, 1)    // impure operand must still run
    }
}

[test]
def test_add_sub_identities(t : T?) {
    t |> run("x+0 and x-0") @(t : T?) {
        t |> equal(g_i + 0, 7)
        t |> equal(0 + g_i, 7)
        t |> equal(g_i - 0, 7)
        t |> equal(g_f + 0.0, 3.5)
        t |> equal(g_f - 0.0, 3.5)
        t |> equal(g_d + 0.0lf, 2.25lf)
        t |> equal(g_v + float3(0.0), float3(1.0, 2.0, 4.0))
    }
    t |> run("0-x negates") @(t : T?) {
        t |> equal(0 - g_i, -7)
        t |> equal(0.0 - g_f, -3.5)
    }
    t |> run("x-x is zero for ints") @(t : T?) {
        t |> equal(g_i - g_i, 0)
        t |> equal(g_i64 - g_i64, 0l)
    }
}

[test]
def test_div_mod_identities(t : T?) {
    t |> equal(g_i / 1, 7)
    t |> equal(g_f / 1.0, 3.5)
    t |> equal(g_v / 1.0, float3(1.0, 2.0, 4.0))
    t |> equal(g_i % 1, 0)
    g_calls = 0
    t |> equal(side_i() % 1, 0)
    t |> equal(g_calls, 1)
}

[test]
def test_bitwise_identities(t : T?) {
    t |> equal(g_u & 0u, 0u)
    t |> equal(g_u | 0u, 0xDEADu)
    t |> equal(g_u ^ 0u, 0xDEADu)
    t |> equal(g_u ^ g_u, 0u) // nolint:LINT007 -- deliberate: exercises the x^x fold
    t |> equal(g_u & g_u, 0xDEADu) // nolint:LINT007 -- deliberate: exercises the x&x fold
    t |> equal(g_u | g_u, 0xDEADu) // nolint:LINT007 -- deliberate: exercises the x|x fold
    t |> equal(g_u << 0u, 0xDEADu)
    t |> equal(g_u >> 0u, 0xDEADu)
    t |> equal(g_i << 0, 7)
}

[test]
def test_bool_identities(t : T?) {
    t |> run("const arms fold") @(t : T?) {
        // both values of g_b — with only g_b==true, a fold that wrongly rewrote
        // `b && true` to constant `true` would pass undetected
        t |> equal(g_b && true, true)
        t |> equal(g_b && false, false)
        t |> equal(g_b || false, true)
        t |> equal(g_b || true, true)
        t |> equal(true && g_b, true)
        t |> equal(false || g_b, true)
        g_b = false
        t |> equal(g_b && true, false)
        t |> equal(g_b && false, false)
        t |> equal(g_b || false, false)
        t |> equal(g_b || true, true)
        t |> equal(true && g_b, false)
        t |> equal(false || g_b, false)
        g_b = true
    }
    t |> run("short-circuit never evaluates right of const-false") @(t : T?) {
        g_calls = 0
        t |> equal(false && side_b(), false)
        t |> equal(true || side_b(), true)
        t |> equal(g_calls, 0)
    }
    t |> run("purity gate keeps impure left evaluated") @(t : T?) {
        g_calls = 0
        t |> equal(side_b() && false, false)
        t |> equal(g_calls, 1)
        t |> equal(side_b() || true, true)
        t |> equal(g_calls, 2)
    }
    t |> run("double negation and compare flip") @(t : T?) {
        t |> equal(!!g_b, true)
        t |> equal(!(g_i < 5), g_i >= 5)
        t |> equal(!(g_i == 7), false)
        t |> equal(-(-g_i), 7)
    }
}

[test]
def test_ternary_identities(t : T?) {
    t |> equal(g_i > 3 ? true : false, true)
    t |> equal(g_i > 3 ? false : true, false)
    t |> equal(g_i < 3 ? g_f : g_f, 3.5) // nolint:LINT008 -- deliberate: exercises the c?a:a fold
}

[test]
def test_same_type_casts(t : T?) {
    t |> equal(int(g_i), 7) // nolint:PERF020 -- deliberate: this whole test exercises the same-type cast fold
    t |> equal(uint(g_u), 0xDEADu) // nolint:PERF020
    t |> equal(int64(g_i64), 9000000000l) // nolint:PERF020
    t |> equal(float(g_f), 3.5) // nolint:PERF020
    t |> equal(double(g_d), 2.25lf) // nolint:PERF020
    t |> equal(float3(g_v), float3(1.0, 2.0, 4.0))
}

[test]
def test_splat_collapse(t : T?) {
    t |> equal(g_v * float3(g_f), float3(1.0 * g_f, 2.0 * g_f, 4.0 * g_f))
    t |> equal(g_v / float3(g_f), float3(1.0 / g_f, 2.0 / g_f, 4.0 / g_f))
    t |> equal(float3(g_f) * g_v, float3(g_f * 1.0, g_f * 2.0, g_f * 4.0))
}

[test]
def test_dead_loops(t : T?) {
    g_calls = 0
    while (false) {
        g_calls++
    }
    for (_i in range(0, 0)) {
        g_calls++
    }
    for (_i in range(3, 3)) {
        g_calls++
    }
    for (_i in urange(5u, 5u)) {
        g_calls++
    }
    t |> equal(g_calls, 0)
}

[test]
def test_finalizer_only_function_still_runs(t : T?) {
    // regression: isNop used to treat a finalizer-only body as a nop and delete the
    // call, silently dropping the defer's side effect under optimization
    g_calls = 0
    only_finally()
    t |> equal(g_calls, 1)
}

[test]
def test_cond_fold_keeps_impure_cond(t : T?) {
    // regression: `if (impure()) return; else return;` folded to a bare return,
    // silently dropping the condition's side effect
    g_calls = 0
    void_return_impure()
    t |> equal(g_calls, 1)
}

[test]
def test_elif_chain_values(t : T?) {
    // the elif chain folds to nested ternaries (shape proof in optimization_fold_shape.das);
    // every branch must keep its value
    t |> equal(elif_grade(11), 3)
    t |> equal(elif_grade(7), 2)
    t |> equal(elif_grade(1), 1)
    t |> equal(elif_grade(-5), 0)
}

[test]
def test_cond_fold_keeps_arm_finally(t : T?) {
    // regression: an arm holding one return plus a finally section (defer) used to
    // fold to a ternary return, silently dropping the finally
    g_calls = 0
    t |> equal(defer_in_arm(true), 1)
    t |> equal(g_calls, 1)
    t |> equal(defer_in_arm(false), 2)
    t |> equal(g_calls, 1)
}

[test]
def test_reassociation_off_by_default(t : T?) {
    // without fast_math the chain is NOT regrouped; values are exact here either way
    t |> equal((g_f + 1.0) + 2.0, 6.5)
    t |> equal((g_f * 2.0) * 4.0, 28.0)
    t |> equal(2.0 * (g_f * 4.0), 28.0)
}
