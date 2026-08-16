options gen2
options rtti
options disable_auto_inline    // this file pins DSE shapes; auto-splicing would dissolve them

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require dastest/testing_boost public
require strings
require daslib/strings_boost

// structural FIRED proof + behavioral safety net for the dead-store-elimination pass
// (src/ast/ast_dse.cpp): a store to a plain local is removed when nothing reads it
// before the next full overwrite; an impure rhs survives as a bare statement. The
// behavioral tests pin the safety gates: aliased/by-ref locals untouched, op-assign
// is a read (gen) but never a candidate, functions with finally skipped whole.

var g_count = 0
var g_obs = 0

def bump() : int {
    g_count ++
    return g_count
}

def call_it(blk : block) {
    invoke(blk)
}

[export]
def target_overwrite(x : int) : int {
    var t = 0 // nolint:LINT010 -- deliberate dead init
    t = x * 5 // nolint:LINT010 -- dead: overwritten below before any read
    t = x + 7
    return t
}

[export]
def target_trailing(x : int) : int {
    var t = x + 1
    let r = t * 2
    t = x * 9 // nolint:LINT010 -- dead: t never read again
    return r
}

[export]
def target_branch_dead(x : int; c : bool) : int {
    var t = 0 // nolint:LINT010 -- deliberate dead init
    t = x * 11 // dead: overwritten on both arms before any read
    if (c) {
        t = x + 1
    } else {
        t = x + 2
    }
    return t
}

[export]
def target_branch_live(x : int; c : bool) : int {
    var t = 0 // nolint:LINT010 -- deliberate dead init
    t = x + 3 // LIVE: read on the then-path
    if (c) {
        return t
    }
    return x
}

[export]
def target_loop_live(x : int) : int {
    var acc = 0
    var i = 0
    while (i < x) {
        acc = acc + i // LIVE: read next iteration and after the loop
        i = i + 1     // LIVE: read by the condition
    }
    return acc
}

[export]
def target_chain(x : int) : int {
    var a = 0 // nolint:LINT010 -- deliberate dead init
    var b = 0 // nolint:LINT010 -- deliberate dead init
    a = x + 13 // nolint:LINT010 -- dead: only read by the dead store below
    b = a * 2 // nolint:LINT010 -- dead: b never read
    return x
}

[export]
def target_impure(x : int) : int {
    var t = 0
    t = bump() // target is dead, rhs has side effects — the store drops, the call stays
    return x
}

[export]
def target_impure_reads(x : int) : int {
    var u = 0 // nolint:LINT010 -- deliberate dead init
    u = x * 109 // LIVE: the kept bare rhs below still reads u
    var t = 0 // nolint:LINT010 -- deliberate dead init
    t = bump() + u // nolint:LINT010 -- dead store, but operator-topped impure rhs: stays WHOLE
    return x
}

[export]
def target_alias(x : int) : int {
    var t = 0
    t = x + 5
    var p = unsafe(addr(t))
    t = x * 6 // looks dead by direct uses, but p aliases t — addr() disqualifies t
    return *p
}

[export]
def target_string() : string {
    var s = "" // nolint:LINT010 -- deliberate dead init
    s = "deadbeef" // nolint:LINT010 -- dead: overwritten before any read
    s = "live"
    return s
}

[export]
def target_ptr(a, b : int const?) : int const? {
    var p : int const?
    p = a // nolint:LINT010,STYLE011 -- dead: overwritten before any read; decl-then-assign is the shape under test
    p = b
    return p
}

[export]
def target_capture(x : int) : int {
    var t = 0 // nolint:LINT010 -- deliberate dead init
    var r = 0
    t = x + 40 // LIVE: read inside the block below
    call_it() $ {
        r = t + 2
    }
    return r
}

[export]
def target_finally(x : int) : int {
    var t = 0 // nolint:LINT010 -- deliberate dead init
    {
        t = x + 21 // LIVE: the finally reads it on the early-return path the CFG does not model
        return x
    } finally {
        g_obs = t
    }
    return 0
}

[export]
def target_opassign(x : int) : int {
    var t = x
    t += 1 // nolint:PERF013 -- op-assign is never a removal candidate; the `+=` shape is the point
    return x
}

[export]
def target_opassign_gen(x : int) : int {
    var t = x
    t += 3 // gen: reads t, so the init above stays live
    let r = t * 2
    t = x * 93 // nolint:LINT010 -- dead: t never read again (op-assign no longer disqualifies)
    return r
}

[export]
def target_opassign_reads(x : int) : int {
    var t = 0 // nolint:LINT010 -- deliberate dead init
    t = x * 97 // LIVE: the op-assign below reads it
    t += 5
    return t
}

[export]
def target_increment_gen(x : int) : int {
    var t = x
    t ++ // gen: reads t, so the init above stays live
    let r = t * 4
    t = x * 107 // nolint:LINT010 -- dead: t never read again
    return r
}

// ----- v2: field-chain stores and declaration inits (ast_alias.h) -----

struct DsePoint {
    x : int
    y : int
}

struct DseBody {
    pos : DsePoint
    id : int
}

def observe_field(s : DseBody) : int => s.pos.x

[export]
def target_opassign_field(x : int) : int {
    var s : DseBody
    s.pos.x = x + 101 // LIVE: the op-assign below reads it
    s.pos.x += 7
    s.id = x * 103 // dead: id never read
    return s.pos.x
}

[export]
def target_field_overwrite(x : int) : int {
    var s : DseBody
    s.pos.x = x * 51 // dead: overwritten below before any read
    s.pos.x = x + 53
    return s.pos.x
}

[export]
def target_field_disjoint(x : int) : int {
    var s : DseBody
    s.pos.x = x + 57 // LIVE: disjoint fields never kill each other
    s.pos.y = x + 59
    return s.pos.x + s.pos.y
}

[export]
def target_field_partial(x : int) : int {
    var s : DseBody
    s.pos = DsePoint(x = x + 61, y = x + 67) // LIVE: its y survives the partial overwrite below
    s.pos.x = x + 71
    return s.pos.x + s.pos.y
}

[export]
def target_whole_read(x : int) : int {
    var s : DseBody
    s.pos.x = x + 73 // LIVE: the whole-struct read below observes it
    return observe_field(s)
}

[export]
def target_arg_store(var s : DseBody; x : int) {
    s.pos.x = x + 79 // LIVE: argument writes are the caller's to observe
}

[export]
def target_init_strip(x : int) : int {
    var t = x * 83 // dead init: overwritten on every path before any read
    if (x > 0) {
        t = x + 1
    } else {
        t = x + 2
    }
    return t
}

[export]
def target_init_live(x : int) : int {
    var t = x * 89 // LIVE init: read on the else path
    if (x > 0) {
        t = x + 3
    }
    return t
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
    // statements whose arguments collide with the substrings asserted here — drop them
    let clean <- [for (ln in split(body, "\n")); ln; where !(ln |> has("coverage("))]
    return join(clean, "\n")
}

[test]
def test_dead_stores_fired(t : T?) {
    ast_gc_guard() {
        t |> run("overwritten store is removed") @(t : T?) {
            let b = body_of(t, "target_overwrite")
            t |> success(!has(b, "* 5"), "dead store gone: {b}")
            t |> success(has(b, "+ 7"), "live store kept: {b}")
        }
        t |> run("store after the last read is removed") @(t : T?) {
            let b = body_of(t, "target_trailing")
            t |> success(!has(b, "* 9"), "trailing dead store gone: {b}")
            t |> success(has(b, "* 2"), "live read kept: {b}")
        }
        t |> run("store killed on both branch arms is removed") @(t : T?) {
            let b = body_of(t, "target_branch_dead")
            t |> success(!has(b, "* 11"), "pre-branch dead store gone: {b}")
            t |> success(has(b, "+ 1") && has(b, "+ 2"), "arm stores kept: {b}")
        }
        t |> run("store live on one path only is kept") @(t : T?) {
            let b = body_of(t, "target_branch_live")
            t |> success(has(b, "+ 3"), "partially-live store kept: {b}")
        }
        t |> run("loop-carried stores are kept") @(t : T?) {
            let b = body_of(t, "target_loop_live")
            t |> success(has(b, "+ i"), "accumulator kept: {b}")
            t |> success(has(b, "+ 1"), "counter kept: {b}")
        }
        t |> run("dead-store chain falls in one pass") @(t : T?) {
            let b = body_of(t, "target_chain")
            t |> success(!has(b, "+ 13") && !has(b, "* 2"), "whole chain gone: {b}")
        }
        t |> run("impure rhs survives as a bare call") @(t : T?) {
            let b = body_of(t, "target_impure")
            t |> success(has(b, "bump"), "side-effecting rhs kept: {b}")
            t |> success(!has(b, "= bump"), "dead store stripped to a bare call: {b}")
        }
        t |> run("operator-topped impure rhs keeps its store whole") @(t : T?) {
            // stripping to a bare `bump() + u` would trip the top-level no-side-effect lint
            // when a reused shared module re-lints its optimized AST — the store must survive
            let b = body_of(t, "target_impure_reads")
            t |> success(has(b, "* 109"), "store read by the kept rhs stays: {b}")
            t |> success(has(b, "= bump"), "operator-topped dead store kept whole: {b}")
        }
        t |> run("addr() disqualifies the variable") @(t : T?) {
            let b = body_of(t, "target_alias")
            t |> success(has(b, "* 6"), "aliased store kept: {b}")
        }
        t |> run("dead string store is removed") @(t : T?) {
            let b = body_of(t, "target_string")
            t |> success(!has(b, "deadbeef"), "dead string store gone: {b}")
            t |> success(has(b, "live"), "live string store kept: {b}")
        }
        t |> run("dead pointer store is removed") @(t : T?) {
            let b = body_of(t, "target_ptr")
            t |> success(!has(b, "p = a"), "dead pointer store gone: {b}")
            t |> success(has(b, "p = b"), "live pointer store kept: {b}")
        }
        t |> run("store read inside a block capture is kept") @(t : T?) {
            let b = body_of(t, "target_capture")
            t |> success(has(b, "+ 40"), "block-read store kept: {b}")
        }
        t |> run("function with finally is skipped whole") @(t : T?) {
            let b = body_of(t, "target_finally")
            t |> success(has(b, "+ 21"), "store read by finally kept: {b}")
        }
        t |> run("op-assign alone is never a removal candidate") @(t : T?) {
            let b = body_of(t, "target_opassign")
            t |> success(has(b, "+="), "op-assign statement kept: {b}")
        }
        t |> run("op-assign no longer disqualifies: later dead store falls") @(t : T?) {
            let b = body_of(t, "target_opassign_gen")
            t |> success(!has(b, "* 93"), "dead store after op-assign gone: {b}")
            t |> success(has(b, "+= 3"), "op-assign kept: {b}")
        }
        t |> run("op-assign reads keep the store they observe") @(t : T?) {
            let b = body_of(t, "target_opassign_reads")
            t |> success(has(b, "* 97"), "store read by op-assign stays: {b}")
        }
        t |> run("increment no longer disqualifies: later dead store falls") @(t : T?) {
            let b = body_of(t, "target_increment_gen")
            t |> success(!has(b, "* 107"), "dead store after increment gone: {b}")
        }
        t |> run("field op-assign: gen on its chain, disjoint dead store falls") @(t : T?) {
            let b = body_of(t, "target_opassign_field")
            t |> success(has(b, "+ 101"), "store read by field op-assign stays: {b}")
            t |> success(!has(b, "* 103"), "disjoint dead field store gone: {b}")
        }
        t |> run("overwritten field store is removed") @(t : T?) {
            let b = body_of(t, "target_field_overwrite")
            t |> success(!has(b, "* 51"), "dead field store gone: {b}")
            t |> success(has(b, "+ 53"), "live field store kept: {b}")
        }
        t |> run("disjoint fields never kill each other") @(t : T?) {
            let b = body_of(t, "target_field_disjoint")
            t |> success(has(b, "+ 57") && has(b, "+ 59"), "both field stores kept: {b}")
        }
        t |> run("partial overwrite keeps the whole-chain store") @(t : T?) {
            let b = body_of(t, "target_field_partial")
            t |> success(has(b, "+ 67"), "prefix store kept for its y: {b}")
            t |> success(has(b, "+ 71"), "partial store kept: {b}")
        }
        t |> run("whole-struct read keeps the field store") @(t : T?) {
            let b = body_of(t, "target_whole_read")
            t |> success(has(b, "+ 73"), "field store kept: {b}")
        }
        t |> run("argument stores are never candidates") @(t : T?) {
            let b = body_of(t, "target_arg_store")
            t |> success(has(b, "+ 79"), "argument store kept: {b}")
        }
        t |> run("dead declaration init is stripped") @(t : T?) {
            let b = body_of(t, "target_init_strip")
            t |> success(!has(b, "* 83"), "dead init gone: {b}")
            t |> success(has(b, "+ 1") && has(b, "+ 2"), "arm stores kept: {b}")
        }
        t |> run("init read on one path is kept") @(t : T?) {
            let b = body_of(t, "target_init_live")
            t |> success(has(b, "* 89"), "live init kept: {b}")
        }
    }
}

[test]
def test_dead_stores_behavior(t : T?) {
    t |> run("values preserved") @(t : T?) {
        t |> equal(target_overwrite(3), 10)
        t |> equal(target_trailing(4), 10)
        t |> equal(target_branch_dead(5, true), 6)
        t |> equal(target_branch_dead(5, false), 7)
        t |> equal(target_branch_live(2, true), 5)
        t |> equal(target_branch_live(2, false), 2)
        t |> equal(target_loop_live(4), 6)
        t |> equal(target_chain(15), 15)
        t |> equal(target_string(), "live")
        t |> equal(target_capture(1), 43)
        t |> equal(target_opassign(5), 5)
    }
    t |> run("side effects of a dead target's rhs still happen") @(t : T?) {
        g_count = 0
        t |> equal(target_impure(1), 1)
        t |> equal(g_count, 1)
        t |> equal(target_impure(2), 2)
        t |> equal(g_count, 2)
        t |> equal(target_impure_reads(3), 3)
        t |> equal(g_count, 3)
    }
    t |> run("op-assign values preserved") @(t : T?) {
        t |> equal(target_opassign_gen(5), 16)
        t |> equal(target_opassign_reads(3), 296)
        t |> equal(target_increment_gen(2), 12)
        t |> equal(target_opassign_field(1), 109)
    }
    t |> run("aliased store still lands") @(t : T?) {
        t |> equal(target_alias(7), 42)
    }
    t |> run("finally observes the store on the early-return path") @(t : T?) {
        g_obs = 0
        t |> equal(target_finally(9), 9)
        t |> equal(g_obs, 30)
    }
    t |> run("pointer store chain") @(t : T?) {
        var va = 1
        var vb = 2
        let res = target_ptr(unsafe(addr(va)), unsafe(addr(vb)))
        t |> equal(*res, 2)
    }
    t |> run("chain values preserved") @(t : T?) {
        t |> equal(target_field_overwrite(2), 55)
        t |> equal(target_field_disjoint(1), 118)
        t |> equal(target_field_partial(1), 140)
        t |> equal(target_whole_read(1), 74)
        t |> equal(target_init_strip(4), 5)
        t |> equal(target_init_strip(-4), -2)
        t |> equal(target_init_live(4), 7)
        t |> equal(target_init_live(-4), -356)
    }
    t |> run("argument store lands in the caller") @(t : T?) {
        var s : DseBody
        target_arg_store(s, 1)
        t |> equal(s.pos.x, 80)
    }
}
