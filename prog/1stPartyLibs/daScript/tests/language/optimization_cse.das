options gen2
options rtti
options no_coverage
options disable_auto_inline    // this file pins CSE shapes; auto-splicing would dissolve them

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require dastest/testing_boost public
require strings
require daslib/strings_boost

// structural FIRED proof + behavioral safety net for common-subexpression elimination
// (src/ast/ast_cse.cpp): a pure expression already held by a clean local (`let t = E`
// or statement-level `t = E`) is reused where it is available on every path and t is
// lexically visible. The negative tests pin the safety gates: kills on stores to
// inputs, one-arm-only availability, closed lexical scopes, impure calls, aliased
// leaves, and make-block opacity.

var g_count = 0

def bump() : int {
    g_count ++
    return g_count
}

def call_it(blk : block) {
    invoke(blk)
}

def sq(v : int) : int => v * v

[export]
def target_let_reuse(x, y : int) : int {
    let a = x * y
    let b = x * y // reused: becomes b = a
    return a + b
}

[export]
def target_copy_reuse(x, y : int) : int {
    var t = 0 // nolint:LINT010 -- dead init, DSE fodder
    t = x * y
    let u = x * y // reused: becomes u = t
    return t + u
}

[export]
def target_arg_reuse(x, y : int) : int {
    let a = x * y
    return x * y + a // reused: becomes a + a
}

[export]
def target_chain(x, y : int) : int {
    let a = x * y
    let b = x * y + 3 // inner x*y reused from a
    let c = x * y + 3 // whole rhs reused from b
    return a + b + c
}

[export]
def target_kill(x : int) : int {
    var y = x + 1
    let a = y * 3
    y = x + 2     // kills (y*3 -> a)
    let b = y * 3 // must be recomputed
    return a + b
}

[export]
def target_branch_avail(x : int; c : bool) : int {
    var t = 0 // nolint:LINT010 -- dead init, DSE fodder
    if (c) { // nolint:LINT009 -- identical arms are the point: pair available on both paths
        t = x * 7
    } else {
        t = x * 7
    }
    return x * 7 + t // available on both paths: becomes t + t
}

[export]
def target_branch_partial(x : int; c : bool) : int {
    var t = 0
    if (c) {
        t = x * 5
    }
    return x * 5 + t // NOT available on the else path: kept
}

[export]
def target_scope(x : int) : int {
    var r = 0 // nolint:LINT010 -- deliberate dead init
    {
        let a = x * 13
        r = a + 1
    }
    // BLOCK FOLDING flattens the bare scope before CSE runs, so this reuse is legal
    // (a is function-scope by then). The pass's own lexical-scope check stays as the
    // soundness belt for shapes block folding does not dissolve.
    return x * 13 + r
}

[export]
def target_loop(x : int) : int {
    var acc = 0
    var i = 0
    while (i < x) {
        let s = i * x
        acc = acc + s + i * x // same iteration, no kill between: becomes s + s
        i = i + 1             // kills (i*x -> s) on the back edge
    }
    return acc
}

[export]
def target_impure(x : int) : int {
    let a = bump()
    let b = bump() // impure: must stay a real call
    return a + b + x
}

[export]
def target_pure_call(x : int) : int {
    let a = sq(x + 3)
    return sq(x + 3) + a // pure user call reused: becomes a + a
}

[export]
def target_ternary(x : int; c : bool) : int {
    let a = c ? x + 1 : x - 1
    return (c ? x + 1 : x - 1) * a // whole ternary reused: becomes a * a
}

[export]
def target_cast(x : int) : float {
    let a = float(x) * 0.5
    return float(x) * 0.5 + a // cast tree reused: becomes a + a
}

[export]
def target_swizzle(v : float3) : float {
    let a = v.x + v.y
    return v.x + v.y + a // value-read swizzles over a by-value leaf: becomes a + a
}

[export]
def target_alias(x : int) : int {
    var y = x + 1
    let a = y * 3
    var p = unsafe(addr(y)) // addr() disqualifies y as a leaf
    let b = y * 3           // must be recomputed
    return a + b + *p
}

[export]
def target_ptr_factory(td : TypeDeclPtr) : bool {
    let a = clone_type(td)
    let b = clone_type(td) // pure but identity-producing: must stay a real call
    return a != b
}

[export]
def target_blockarg(x : int) : int {
    let a = x * 17
    var r = 0
    call_it() $ {
        r = x * 17 // inside a make-block: opaque, never rewritten
    }
    return r + a
}

// ----- v2: field/index chains rooted at locals and arguments (ast_alias.h) -----

struct CsePoint {
    x : int
    y : int
}

struct CseBody {
    pos : CsePoint
    id : int
}

def touch(var b : CseBody) {
    b.pos.x ++
}

[export]
def target_field_reuse(b : CseBody) : int {
    let a = b.pos.x * 19
    return b.pos.x * 19 + a // chain reused: becomes a + a
}

[export]
def target_field_kill(var b : CseBody) : int {
    let a = b.pos.x * 23
    b.pos.x = 9   // visible store through the base kills the pair
    return b.pos.x * 23 + a
}

[export]
def target_field_opaque(var b : CseBody) : int {
    let a = b.pos.x * 29
    touch(b)      // impure call: kills every memory pair
    return b.pos.x * 29 + a
}

[export]
def target_index_reuse(arr : array<int>; i : int) : int {
    let a = arr[i] * 31
    return arr[i] * 31 + a // index chain reused: becomes a + a
}

[export]
def target_index_kill(var arr : array<int>; i : int) : int {
    let a = arr[i] * 37
    arr |> push(1) // argument-writing call kills chains rooted at arr
    return arr[i] * 37 + a
}

[export]
def target_local_precision(x : int) : int {
    let s1 = CseBody(pos = CsePoint(x = x, y = 0), id = 0)
    var s2 = CseBody(pos = CsePoint(x = x + 1, y = 0), id = 0)
    let a = s1.pos.x * 41
    s2.pos.x = 7  // a store through ANOTHER local cannot alias s1: the pair survives
    return s1.pos.x * 41 + a + s2.pos.x
}

[export]
def target_arg_aliasing(var a : CseBody; b : CseBody) : int {
    let t = b.pos.x * 43
    a.pos.x = 5   // arguments may alias each other: kills argument-rooted pairs
    return b.pos.x * 43 + t
}

[export]
def target_pure_ref_read(arr : array<int>) : int {
    let n = length(arr) * 47
    return length(arr) * 47 + n // pure by-ref read of a chain base reused
}

def has(s, sub : string) : bool {
    return find(s, sub, 0) >= 0
}

def count_of(s, sub : string) : int {
    return length(split(s, sub)) - 1
}

def body_of(t : T?; nm : string) : string {
    var body = ""
    compiling_module() |> for_each_function("{nm}") $(func) {
        if (empty(body)) {
            body = "{describe(func.body)}"
        }
    }
    t |> success(!empty(body), "target function found")
    // this module opts out of coverage injection (options no_coverage above) — injected
    // add_line_coverage calls would make pure user callees impure and defeat the pass
    // under the coverage lane; the filter below stays as a belt for injected statements
    let clean <- [for (ln in split(body, "\n")); ln; where !(ln |> has("coverage("))]
    return join(clean, "\n")
}

[test]
def test_cse_fired(t : T?) {
    ast_gc_guard() {
        t |> run("let-seeded reuse") @(t : T?) {
            let b = body_of(t, "target_let_reuse")
            t |> equal(count_of(b, "* y"), 1)
        }
        t |> run("statement-copy-seeded reuse") @(t : T?) {
            let b = body_of(t, "target_copy_reuse")
            t |> equal(count_of(b, "* y"), 1)
        }
        t |> run("reuse inside a return expression") @(t : T?) {
            let b = body_of(t, "target_arg_reuse")
            t |> equal(count_of(b, "* y"), 1)
            t |> success(has(b, "a + a"), "rewritten to the holder: {b}")
        }
        t |> run("nested and chained reuse") @(t : T?) {
            let b = body_of(t, "target_chain")
            t |> equal(count_of(b, "* y"), 1)
            t |> equal(count_of(b, "+ 3"), 1)
        }
        t |> run("store to an input kills the pair") @(t : T?) {
            let b = body_of(t, "target_kill")
            t |> equal(count_of(b, "* 3"), 2)
        }
        t |> run("available on both branch arms is reused") @(t : T?) {
            let b = body_of(t, "target_branch_avail")
            t |> equal(count_of(b, "* 7"), 2)
            t |> success(has(b, "t + t"), "merge use rewritten: {b}")
        }
        t |> run("available on one arm only is kept") @(t : T?) {
            let b = body_of(t, "target_branch_partial")
            t |> equal(count_of(b, "* 5"), 2)
        }
        t |> run("bare scope flattens before CSE, reuse is legal") @(t : T?) {
            let b = body_of(t, "target_scope")
            t |> equal(count_of(b, "* 13"), 1)
        }
        t |> run("loop-body reuse within one iteration") @(t : T?) {
            let b = body_of(t, "target_loop")
            t |> equal(count_of(b, "i * x"), 1)
        }
        t |> run("impure call is never reused") @(t : T?) {
            let b = body_of(t, "target_impure")
            t |> equal(count_of(b, "bump()"), 2)
        }
        t |> run("pure user call is reused") @(t : T?) {
            let b = body_of(t, "target_pure_call")
            t |> equal(count_of(b, "sq("), 1)
        }
        t |> run("ternary tree is reused") @(t : T?) {
            let b = body_of(t, "target_ternary")
            t |> equal(count_of(b, " ? "), 1)
        }
        t |> run("cast tree is reused") @(t : T?) {
            let b = body_of(t, "target_cast")
            t |> equal(count_of(b, "float(x)"), 1)
        }
        t |> run("value-read swizzles are reused") @(t : T?) {
            let b = body_of(t, "target_swizzle")
            t |> equal(count_of(b, "v.x"), 1)
        }
        t |> run("addr() disqualifies the leaf") @(t : T?) {
            let b = body_of(t, "target_alias")
            t |> equal(count_of(b, "* 3"), 2)
        }
        t |> run("make-block interiors are opaque") @(t : T?) {
            let b = body_of(t, "target_blockarg")
            t |> equal(count_of(b, "* 17"), 2)
        }
        t |> run("pointer factories are never merged") @(t : T?) {
            // clone_type is side-effect-free but returns a FRESH node per call -
            // reusing one evaluation would alias a uniquely-owned TypeDecl
            let b = body_of(t, "target_ptr_factory")
            t |> equal(count_of(b, "clone_type"), 2)
            var td = new TypeDecl(baseType = Type.tInt)
            t |> success(target_ptr_factory(td), "fresh clones are distinct")
        }
        t |> run("field chain is reused") @(t : T?) {
            let b = body_of(t, "target_field_reuse")
            t |> equal(count_of(b, "* 19"), 1)
        }
        t |> run("store through the base kills the chain pair") @(t : T?) {
            let b = body_of(t, "target_field_kill")
            t |> equal(count_of(b, "* 23"), 2)
        }
        t |> run("impure call kills memory pairs") @(t : T?) {
            let b = body_of(t, "target_field_opaque")
            t |> equal(count_of(b, "* 29"), 2)
        }
        t |> run("index chain is reused") @(t : T?) {
            let b = body_of(t, "target_index_reuse")
            t |> equal(count_of(b, "* 31"), 1)
        }
        t |> run("argument-writing call kills the array chain") @(t : T?) {
            let b = body_of(t, "target_index_kill")
            t |> equal(count_of(b, "* 37"), 2)
        }
        t |> run("store through another local keeps the pair") @(t : T?) {
            let b = body_of(t, "target_local_precision")
            t |> equal(count_of(b, "* 41"), 1)
        }
        t |> run("store through one argument kills pairs on another") @(t : T?) {
            let b = body_of(t, "target_arg_aliasing")
            t |> equal(count_of(b, "* 43"), 2)
        }
        t |> run("pure by-ref read is reused") @(t : T?) {
            let b = body_of(t, "target_pure_ref_read")
            t |> equal(count_of(b, "* 47"), 1)
        }
    }
}

[test]
def test_cse_behavior(t : T?) {
    t |> run("values preserved") @(t : T?) {
        t |> equal(target_let_reuse(3, 4), 24)
        t |> equal(target_copy_reuse(3, 4), 24)
        t |> equal(target_arg_reuse(3, 4), 24)
        t |> equal(target_chain(3, 4), 42)
        t |> equal(target_kill(5), 39)
        t |> equal(target_branch_avail(2, true), 28)
        t |> equal(target_branch_avail(2, false), 28)
        t |> equal(target_branch_partial(2, true), 20)
        t |> equal(target_branch_partial(2, false), 10)
        t |> equal(target_scope(2), 53)
        t |> equal(target_loop(3), 18)
        t |> equal(target_pure_call(2), 50)
        t |> equal(target_ternary(4, true), 25)
        t |> equal(target_ternary(4, false), 9)
        t |> equal(target_cast(4), 4.)
        t |> equal(target_swizzle(float3(1.5, 2.5, 0.)), 8.)
        t |> equal(target_blockarg(2), 68)
    }
    t |> run("impure calls both happen") @(t : T?) {
        g_count = 0
        t |> equal(target_impure(10), 13)
        t |> equal(g_count, 2)
    }
    t |> run("aliased leaf still recomputes") @(t : T?) {
        t |> equal(target_alias(5), 42)
    }
    t |> run("chain values preserved") @(t : T?) {
        var b = CseBody(pos = CsePoint(x = 3, y = 0), id = 0)
        t |> equal(target_field_reuse(b), 114)
        t |> equal(target_field_kill(b), 276)
        b.pos.x = 3
        t |> equal(target_field_opaque(b), 203)
        var arr <- [2, 5, 8]
        t |> equal(target_index_reuse(arr, 0), 124)
        t |> equal(target_index_kill(arr, 0), 148)
        t |> equal(target_local_precision(2), 171)
        t |> equal(target_pure_ref_read(arr), 376)  // push above grew it to 4
    }
    t |> run("same struct passed to both parameters reads the store") @(t : T?) {
        var s = CseBody(pos = CsePoint(x = 2, y = 0), id = 0)
        t |> equal(target_arg_aliasing(s, s), 301)  // 5*43 + 2*43: the recompute observes the write
        var s2 = CseBody(pos = CsePoint(x = 2, y = 0), id = 0)
        let s3 = CseBody(pos = CsePoint(x = 2, y = 0), id = 0)
        t |> equal(target_arg_aliasing(s2, s3), 172)
    }
}
