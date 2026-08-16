options gen2
options rtti
options no_coverage

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require dastest/testing_boost public
require strings
require daslib/strings_boost

// structural FIRED proof + behavioral safety net for [inline] splicing
// (src/ast/ast_inline.cpp): every direct call to an [inline] function is replaced
// with the callee body during the patch slot. leaf and pure-single-use arguments
// substitute textually; impure arguments bind call-site temps in call order; `var`
// by-value params become mutable local copies; while / elif / && / ?: positions
// lower first. the behavioral tests pin evaluation order and snapshot semantics.

var g_log : string = ""
var g_count = 0

def side(tag : string; v : int) : int {
    g_log += tag;
    return v;
}

struct SVec {
    x : int;
    y : int;
}

[inline]
def add(a, b : int) : int {
    return a + b;
}

[inline]
def clamp01f(x : float) : float {
    return x < 0.0 ? 0.0 : (x > 1.0 ? 1.0 : x); // nolint:PERF015 -- ternaries in the inline body are the point
}

[inline]
def madd(a, b, c : int) : int {
    let t = a * b;
    return t + c;
}

[inline]
def bump(var x : int) : int {
    x++;
    return x;
}

[inline]
def add_g(a : int) : int {
    g_count += 10;
    return a + g_count;
}

[inline]
def vsum(v : SVec) : int {
    return v.x + v.y;
}

[inline]
def wsum(var v : SVec) : int {
    v.x++;
    return v.x + v.y;
}

[inline]
def read_after_write(a : int; var s : SVec) : int {
    s.x = 100;
    return a;
}

[inline]
def set_y(var s : SVec) {
    s.y = 99;
}

[inline]
def sum_range(n : int) : int {
    var s = 0;
    for (k in range(n)) {
        s += k;
    }
    return s;
}

[inline]
def chain_inner(x : int) : int {
    return x + 1;
}

[inline]
def chain_outer(x : int) : int {
    return chain_inner(x) + 2;
}

[inline]
def twice(x : auto) {
    return x + x;
}

[inline]
def guard_ladder(x : int) : int {
    if (x < 0) {
        return -1;
    }
    if (x == 0) {
        return 0;
    }
    return 1;
}

[inline]
def both_arms(x : int) : int {
    if (x > 0) {
        let a = x * 2;
        return a + 1;
    } else {
        let b = x * 3;
        return b - 1;
    }
}

[inline]
def find_first_gt(src : array<int>; k : int) : int {
    for (v in src) {
        if (v > k) {
            return v; // return under a loop: rides the generated flag
        }
    }
    return -1;
}

[inline]
def find_nested(m : array<array<int>>; k : int) : int {
    for (row in m) {
        for (v in row) {
            if (v == k) {
                return 1; // nested loops: the cascade breaks both
            }
        }
    }
    return 0;
}

[inline]
def early_void(var s : SVec; x : int) {
    if (x < 0) {
        return ;
    }
    s.y = x;
}

[inline]
def move_under_loop(n : int) : array<int> {
    for (k in range(n)) {
        if (k == 3) {
            var early <- [k, k]; // nolint:PERF009 -- named-local move-return is the shape under test
            return <- early;
        }
    }
    var full <- [n]; // nolint:PERF009 -- same: pins ExprMove from a typed local through the flag store
    return <- full;
}

[inline]
def loop_ret_finally(n : int) : int {
    var seen = 0;
    {
        for (k in range(n)) {
            if (k == 2) {
                return 100 + seen; // the block's finally must still run on this exit
            }
            seen++;
        }
    } finally {
        g_count = seen;
    }
    return seen;
}

def for_each_hlp(src : array<int>; blk : block<(v : int) : void>) {
    for (v in src) {
        invoke(blk, v);
    }
}

[inline]
def gathered_sum(src : array<int>; k : int) : int {
    // a block literal in the body: stays in-frame, clones with the splice
    var total = 0;
    src |> for_each_hlp() $(v) {
        if (v > k) {
            total += v;
        }
    }
    return total;
}

[export]
def target_basic(x, y : int) : int {
    return add(x, y + 1); // inlines to x + (y + 1), zero temps
}

[export]
def target_fold : float {
    return clamp01f(2.5); // constant arg: folds through the inlined body
}

[export]
def target_multi_stmt(x : int) : int {
    return madd(x, 4, 5);
}

[export]
def target_generic(x : int; f : float) : float {
    return float(twice(x)) + twice(f); // one instance per type, both inlined
}

[export]
def target_while(x : int) : int {
    var i = 0;
    while (madd(i, 1, 0) < x) { // statement-needing callee in a while cond: lowered
        i++;
    }
    return i;
}

[export]
def target_lazy(q : int) : int {
    let c = (q > 0) && (madd(q, 1, 1) > 0); // && right side: lowered to if-form
    let s = q > 0 ? madd(q, 1, 0) : madd(q, 2, 0); // ?: arms: lowered to if-form
    return (c ? 1 : 0) + s;
}

[export]
def target_guard(x : int) : int {
    return guard_ladder(x); // canonicalization makes it terminal: splices flag-free
}

[export]
def target_both_arms(x : int) : int {
    return both_arms(x); // terminal returns in both arms: in-place stores, no flag
}

[export]
def target_loop_ret(src : array<int>; k : int) : int {
    return find_first_gt(src, k); // return under a loop: generated flag + break
}

def has(s, sub : string) : bool {
    return find(s, sub, 0) >= 0;
}

def count_of(s, sub : string) : int {
    return length(split(s, sub)) - 1;
}

def body_of(t : T?; nm : string) : string {
    var body = "";
    compiling_module() |> for_each_function("{nm}") $(func) {
        if (empty(body)) {
            body = "{describe(func.body)}";
        }
    }
    t |> success(!empty(body), "target function found");
    return body;
}

[test]
def test_inline_fired(t : T?) {
    ast_gc_guard() {
        t |> run("expression body with leaf + pure args grafts in place") @(t : T?) {
            let b = body_of(t, "target_basic");
            t |> equal(count_of(b, "add("), 0);
            t |> success(!has(b, "__inl"), "no temps for leaf and pure single-use args: {b}");
        }
        t |> run("constant arg folds through the inlined body") @(t : T?) {
            let b = body_of(t, "target_fold");
            t |> equal(count_of(b, "clamp01f("), 0);
        }
        t |> run("multi-statement body splices") @(t : T?) {
            let b = body_of(t, "target_multi_stmt");
            t |> equal(count_of(b, "madd("), 0);
        }
        t |> run("generic instances inline per instantiation") @(t : T?) {
            let b = body_of(t, "target_generic");
            t |> equal(count_of(b, "twice("), 0);
        }
        t |> run("while condition lowers, call inlines inside the loop") @(t : T?) {
            let b = body_of(t, "target_while");
            t |> equal(count_of(b, "madd("), 0);
        }
        t |> run("lazy operators lower, calls inline in the branches") @(t : T?) {
            let b = body_of(t, "target_lazy");
            t |> equal(count_of(b, "madd("), 0);
        }
        t |> run("guard ladder canonicalizes and splices with no flag") @(t : T?) {
            let b = body_of(t, "target_guard");
            t |> equal(count_of(b, "guard_ladder("), 0);
            t |> success(!has(b, "_ret"), "terminal returns need no flag: {b}");
        }
        t |> run("both-arms-return body splices with in-place stores") @(t : T?) {
            let b = body_of(t, "target_both_arms");
            t |> equal(count_of(b, "both_arms("), 0);
            t |> success(!has(b, "_ret"), "terminal returns need no flag: {b}");
        }
        t |> run("return under a loop splices through the generated flag") @(t : T?) {
            let b = body_of(t, "target_loop_ret");
            t |> equal(count_of(b, "find_first_gt("), 0);
            t |> success(has(b, "_ret"), "loop return rides the flag: {b}");
        }
    }
}

[test]
def test_inline_behavior(t : T?) {
    t |> run("values preserved") @(t : T?) {
        t |> equal(target_basic(5, 6), 12);
        t |> equal(target_fold(), 1.0);
        t |> equal(target_multi_stmt(3), 17);
        t |> equal(target_generic(21, 1.5), 45.0);
        t |> equal(target_while(5), 5);
        t |> equal(target_lazy(41), 42);
    }
    t |> run("body with a block literal splices") @(t : T?) {
        let arr <- [3, 9, 1, 7];
        t |> equal(gathered_sum(arr, 2), 19);
        t |> equal(gathered_sum(arr, 6), 16);
    }
    t |> run("impure args evaluate once, in call order") @(t : T?) {
        g_log = "";
        let r = add(side("a", 1), side("b", 2));
        t |> equal(r, 3);
        t |> equal(g_log, "ab");
    }
    t |> run("impure prefix stays ahead of arg temps") @(t : T?) {
        g_log = "";
        let r = side("p", 10) + add(side("q", 1), side("r", 2));
        t |> equal(r, 13);
        t |> equal(g_log, "pqr");
    }
    t |> run("var by-value param is a private mutable copy") @(t : T?) {
        var q = 41; // nolint:LINT003 -- mutable on purpose: prove the callee's copy never writes back
        t |> equal(bump(q), 42);
        t |> equal(q, 41);
    }
    t |> run("global arg keeps snapshot semantics against global writes") @(t : T?) {
        g_count = 0;
        t |> equal(add_g(g_count), 10);
    }
    t |> run("struct by-ref params alias like the call did") @(t : T?) {
        var v = SVec(x = 3, y = 4);
        t |> equal(vsum(v), 7);
        t |> equal(wsum(v), 8);
        t |> equal(v.x, 4);
    }
    t |> run("pure memory-reading arg binds a temp when the body writes it") @(t : T?) {
        var v = SVec(x = 7, y = 0);
        t |> equal(read_after_write(v.x, v), 7);
        t |> equal(v.x, 100);
    }
    t |> run("void inline as a statement") @(t : T?) {
        var v = SVec(x = 1, y = 2);
        set_y(v);
        t |> equal(v.y, 99);
    }
    t |> run("callee locals and loop vars rename around caller names") @(t : T?) {
        let s = 1000;
        let k = 2000;
        t |> equal(sum_range(4), 6);
        t |> equal(s, 1000);
        t |> equal(k, 2000);
    }
    t |> run("nested calls and inline chains") @(t : T?) {
        t |> equal(add(add(1, 2), add(3, 4)), 10);
        t |> equal(chain_outer(5), 8);
    }
    t |> run("statement-needing callee in if and elif conditions") @(t : T?) {
        var hits = 0;
        if (madd(1, 2, 3) == 5) {
            hits += 1;
        }
        if (false) {
            hits += 100;
        } elif (madd(1, 1, 1) == 2) {
            hits += 1;
        }
        t |> equal(hits, 2);
    }
    t |> run("statement-needing callee in a for source") @(t : T?) {
        var total = 0;
        for (z in range(0, madd(1, 2, 1))) {
            total += z;
        }
        t |> equal(total, 3);
    }
    t |> run("multi-return values preserved") @(t : T?) {
        t |> equal(target_guard(-5), -1);
        t |> equal(target_guard(0), 0);
        t |> equal(target_guard(7), 1);
        t |> equal(target_both_arms(2), 5);
        t |> equal(target_both_arms(-1), -4);
    }
    t |> run("return under a loop exits at the right element") @(t : T?) {
        let arr <- [3, 9, 1, 7];
        t |> equal(target_loop_ret(arr, 5), 9);
        t |> equal(target_loop_ret(arr, 100), -1);
    }
    t |> run("return under nested loops breaks all the way out") @(t : T?) {
        var m : array<array<int>>;
        var r1 <- [1, 2];
        var r2 <- [3, 4];
        m |> emplace(r1);
        m |> emplace(r2);
        t |> equal(find_nested(m, 4), 1);
        t |> equal(find_nested(m, 9), 0);
    }
    t |> run("void early return skips the write") @(t : T?) {
        var v = SVec(x = 0, y = 7);
        early_void(v, -1);
        t |> equal(v.y, 7);
        early_void(v, 5);
        t |> equal(v.y, 5);
    }
    t |> run("move return under a loop moves into the result") @(t : T?) {
        let a <- move_under_loop(5);
        t |> equal(length(a), 2);
        t |> equal(a[0], 3);
        let b <- move_under_loop(2);
        t |> equal(length(b), 1);
        t |> equal(b[0], 2);
    }
    t |> run("block finally runs on a flagged loop exit") @(t : T?) {
        g_count = -1;
        t |> equal(loop_ret_finally(5), 102);
        t |> equal(g_count, 2);
        g_count = -1;
        t |> equal(loop_ret_finally(1), 1);
        t |> equal(g_count, 1);
    }
    t |> run("user let keeps its metadata through lazy lowering") @(t : T?) {
        // the ?: lowering must hoist to a generated temp, not rewrite the user
        // declaration in place - an in-place rewrite would drop the aka
        let qq = 41;
        let picked aka chosen = qq > 0 ? madd(qq, 1, 0) : madd(qq, 2, 0);
        t |> equal(chosen, 41);
        t |> equal(picked, 41);
    }
}
