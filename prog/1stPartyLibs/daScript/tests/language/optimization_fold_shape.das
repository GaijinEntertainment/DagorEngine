options gen2
options rtti
options no_coverage // shape pins: injected coverage calls legitimately block the folds
options auto_inline_functions = false // shape pins: splices rewrite the very call shapes the folds are probed on

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require daslib/defer
require dastest/testing_boost public
require strings
require daslib/strings_boost

// structural FIRED proof for the ConstFolding algebraic identities: inspects the
// post-optimize AST of the target functions below and asserts the rewrite actually
// happened. the behavioral half (value preservation, purity gates) lives in
// optimization_folding.das — a silently disabled fold passes there but fails here.

[export]
def target_mul_one(x : int) : int {
    return x * 1
}

[export]
def target_add_zero(f : float) : float {
    return f + 0.0
}

[export]
def target_mul_zero(x : int) : int {
    return x * 0
}

[export]
def target_compare_flip(x : int) : bool {
    return !(x < 5)
}

[export]
def target_cast(x : int) : int {
    return int(x) // nolint:PERF020 -- deliberate: the fold under test
}

[export]
def target_not_not(b : bool) : bool {
    return !!b
}

[export]
def target_tern_bool(x : int) : bool {
    return x > 3 ? true : false
}

[export]
def target_dead_while(x : int) : int {
    var acc = x
    while (false) {
        acc ++
    }
    return acc
}

[export]
def target_dead_for(x : int) : int {
    var acc = x
    for (_i in range(3, 3)) {
        acc ++
    }
    return acc
}

[export]
def target_splat(v : float3; s : float) : float3 {
    return v * float3(s)
}

[export]
def target_div_one(f : float) : float {
    return f / 1.0
}

[export]
def target_shift_zero(u : uint) : uint {
    return u << 0u
}

[export]
def target_xor_self(u : uint) : uint {
    return u ^ u // nolint:LINT007 -- deliberate: the fold under test
}

[export]
def target_and_false(b : bool) : bool {
    return b && false
}

[export]
def target_reassoc(f : float) : float {
    return (f + 1.0) + 2.0
}

// ===== Expression::sameAs powered folds (same-shape operands beyond bare vars) =====

struct SamePair {
    a : int
    b : int
}

var g_side = 0

def bump_side() : int {
    g_side ++
    return g_side
}

[export]
def target_xor_self_field(s : SamePair) : int {
    return s.a ^ s.a // nolint:LINT007 -- deliberate: the fold under test
}

[export]
def target_sub_self_swizzle(v : int2) : int {
    return v.x - v.x // nolint:LINT007 -- deliberate: the fold under test
}

[export]
def target_and_self_at(arr : int[4]; i : int) : int {
    return arr[i] & arr[i] // nolint:LINT007 -- deliberate: the fold under test
}

[export]
def target_tern_same_arms(c : bool; s : SamePair) : int {
    return c ? s.b : s.b // nolint:LINT008 -- deliberate: the fold under test
}

[export]
def target_sub_self_impure() : int {
    return bump_side() - bump_side() // nolint:LINT007 -- deliberate: impure operands must NOT fold
}

[export]
def target_sub_self_float(f : float) : float {
    return f - f // nolint:LINT007 -- deliberate: stays without fast_math (inf/NaN)
}

// ===== CondFolding: if/else-return => ternary =====

[export]
def target_elif_chain(x : int) : int {
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

[export]
def target_void_return_pure(b : bool) {
    if (b) { // nolint:LINT009 -- deliberate: equivalent void arms are the fold under test
        return
    } else {
        return
    }
}

[export]
def target_void_return_impure() {
    if (bump_side() > 0) { // nolint:LINT009 -- deliberate: equivalent void arms are the fold under test
        return
    } else {
        return
    }
}

[export]
def target_defer_in_arm(c : bool) : int {
    if (c) {
        defer() {
            g_side++
        }
        return 1
    } else {
        return 2
    }
}

def has(s, sub : string) : bool {
    return find(s, sub, 0) >= 0
}

def body_of(t : T?; nm : string) : string {
    var body = ""
    compiling_module() |> for_each_function("{nm}") $(func) {
        if (empty(body)) {
            body = "{describe(func.body)}"
        }
    }
    t |> success(!empty(body), "target function found")
    // the coverage lane (dastest --cov-path) injects add_func_coverage/add_line_coverage
    // statements whose arguments collide with the substrings asserted here (paths carry
    // `/`, "target_dead_while" carries "while", hex line args can carry "2f") — drop them
    let clean <- [for (ln in split(body, "\n")); ln; where !(ln |> has("coverage("))]
    return join(clean, "\n")
}

[test]
def test_identity_folds_fired(t : T?) {
    ast_gc_guard() {
        t |> run("x*1 leaves no multiply") @(t : T?) {
            let b = body_of(t, "target_mul_one")
            t |> success(!has(b, "*"), "no `*` survives: {b}")
            t |> success(has(b, "return x"), "operand returned directly: {b}")
        }
        t |> run("x+0.0 leaves no add") @(t : T?) {
            let b = body_of(t, "target_add_zero")
            t |> success(!has(b, "+"), "no `+` survives: {b}")
        }
        t |> run("x*0 becomes the zero constant") @(t : T?) {
            let b = body_of(t, "target_mul_zero")
            t |> success(!has(b, "*"), "no `*` survives: {b}")
            t |> success(has(b, "return 0"), "constant zero returned: {b}")
        }
        t |> run("!(a<b) becomes the flipped compare") @(t : T?) {
            let b = body_of(t, "target_compare_flip")
            t |> success(!has(b, "!"), "no `!` survives: {b}")
            t |> success(has(b, ">="), "flipped compare present: {b}")
        }
        t |> run("same-type cast is erased") @(t : T?) {
            let b = body_of(t, "target_cast")
            t |> success(!has(b, "int("), "no cast call survives: {b}")
        }
        t |> run("!!x is erased") @(t : T?) {
            let b = body_of(t, "target_not_not")
            t |> success(!has(b, "!"), "no `!` survives: {b}")
        }
        t |> run("c ? true : false becomes c") @(t : T?) {
            let b = body_of(t, "target_tern_bool")
            t |> success(!has(b, "?"), "no ternary survives: {b}")
        }
        t |> run("while(false) is removed") @(t : T?) {
            let b = body_of(t, "target_dead_while")
            t |> success(!has(b, "while"), "no while survives: {b}")
        }
        t |> run("for over const empty range is removed") @(t : T?) {
            let b = body_of(t, "target_dead_for")
            t |> success(!has(b, "for"), "no for survives: {b}")
        }
        t |> run("v*float3(s) collapses the splat") @(t : T?) {
            let b = body_of(t, "target_splat")
            t |> success(!has(b, "float3("), "no splat ctor survives: {b}")
            t |> success(has(b, "v * s"), "vec*scalar form present: {b}")
        }
        t |> run("x/1.0 leaves no divide") @(t : T?) {
            let b = body_of(t, "target_div_one")
            t |> success(!has(b, "/"), "no `/` survives: {b}")
        }
        t |> run("x<<0 leaves no shift") @(t : T?) {
            let b = body_of(t, "target_shift_zero")
            t |> success(!has(b, "<<"), "no `<<` survives: {b}")
        }
        t |> run("x^x becomes the zero constant") @(t : T?) {
            let b = body_of(t, "target_xor_self")
            t |> success(!has(b, "^"), "no `^` survives: {b}")
        }
        t |> run("x && false becomes false") @(t : T?) {
            let b = body_of(t, "target_and_false")
            t |> success(!has(b, "&&"), "no `&&` survives: {b}")
            t |> success(has(b, "return false"), "constant false returned: {b}")
        }
        t |> run("reassociation stays OFF without fast_math") @(t : T?) {
            let b = body_of(t, "target_reassoc")
            t |> success(has(b, "1f") && has(b, "2f"), "chain not regrouped: {b}")
        }
    }
}

[test]
def test_same_as_folds_fired(t : T?) {
    ast_gc_guard() {
        t |> run("s.a ^ s.a becomes the zero constant") @(t : T?) {
            let b = body_of(t, "target_xor_self_field")
            t |> success(!has(b, "^"), "no `^` survives: {b}")
            t |> success(has(b, "return 0"), "constant zero returned: {b}")
        }
        t |> run("v.x - v.x becomes the zero constant") @(t : T?) {
            let b = body_of(t, "target_sub_self_swizzle")
            t |> success(!has(b, "-"), "no `-` survives: {b}")
            t |> success(has(b, "return 0"), "constant zero returned: {b}")
        }
        t |> run("arr[i] & arr[i] keeps a single read") @(t : T?) {
            let b = body_of(t, "target_and_self_at")
            t |> success(!has(b, "&"), "no `&` survives: {b}")
            t |> success(has(b, "arr[i]"), "single indexed read kept: {b}")
        }
        t |> run("c ? s.b : s.b collapses to the arm") @(t : T?) {
            let b = body_of(t, "target_tern_same_arms")
            t |> success(!has(b, "?"), "no ternary survives: {b}")
            t |> success(has(b, "s.b"), "arm kept: {b}")
        }
        t |> run("impure same-shape operands do NOT fold") @(t : T?) {
            let b = body_of(t, "target_sub_self_impure")
            t |> success(has(b, "bump_side"), "calls kept: {b}")
            t |> success(has(b, "-"), "subtract kept: {b}")
        }
        t |> run("f - f stays OFF without fast_math") @(t : T?) {
            let b = body_of(t, "target_sub_self_float")
            t |> success(has(b, "f - f"), "float self-sub not folded: {b}")
        }
    }
}

[test]
def test_cond_folds_fired(t : T?) {
    ast_gc_guard() {
        t |> run("elif chain of returns collapses fully") @(t : T?) {
            // regression: the inner fold leaves a naked ExprReturn as if_false, which
            // the outer level used to reject (rtti_isBlock) — jamming after one level
            let b = body_of(t, "target_elif_chain")
            t |> success(!has(b, "if"), "no `if` survives: {b}")
            t |> success(has(b, "?"), "ternary chain present: {b}")
        }
        t |> run("void arms with pure cond fold away") @(t : T?) {
            let b = body_of(t, "target_void_return_pure")
            t |> success(!has(b, "if"), "no `if` survives: {b}")
        }
        t |> run("void arms with impure cond keep the call") @(t : T?) {
            // regression: this used to fold to a bare return, dropping the call
            let b = body_of(t, "target_void_return_impure")
            t |> success(has(b, "bump_side"), "condition side effect kept: {b}")
        }
        t |> run("arm with a finally section does not fold") @(t : T?) {
            // regression: `if (c) { return 1 } finally { ... } else { return 2 }` — the
            // shape defer() leaves — used to fold to a ternary, dropping the finally
            let b = body_of(t, "target_defer_in_arm")
            t |> success(has(b, "finally"), "finally section kept: {b}")
        }
    }
}

[test]
def test_same_as_folds_behavior(t : T?) {
    t |> run("values preserved") @(t : T?) {
        t |> equal(target_xor_self_field(SamePair(a = 77, b = 5)), 0)
        t |> equal(target_sub_self_swizzle(int2(9, 4)), 0)
        t |> equal(target_and_self_at(fixed_array(3, 5, 7, 9), 2), 7)
        t |> equal(target_tern_same_arms(true, SamePair(a = 1, b = 42)), 42)
        t |> equal(target_tern_same_arms(false, SamePair(a = 1, b = 42)), 42)
    }
    t |> run("impure operands evaluate exactly twice") @(t : T?) {
        g_side = 0
        t |> equal(target_sub_self_impure(), -1)
        t |> equal(g_side, 2)
    }
}
