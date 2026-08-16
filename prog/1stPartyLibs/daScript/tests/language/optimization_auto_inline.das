options gen2
options rtti
options no_coverage

require daslib/ast
require daslib/rtti
require daslib/ast_boost
require dastest/testing_boost public
require strings
require daslib/strings_boost

// structural FIRED proof + behavioral safety net for automatic block inlining and
// invoke-of-literal devirtualization (src/ast/ast_inline.cpp): a direct call passing a
// block LITERAL argument splices the callee body (best-effort MAY contract - silent
// declines, never errors), and an invoke whose block resolves to a same-function
// literal splices the block body in place. The negative tests pin the decline gates:
// forwarded (non-literal) blocks and recursion stay regular calls.

var g_log : string = ""
var g_probe_pool : array<int>
var g_creatures : array<int>

def side(tag : string; v : int) : int {
    g_log += tag
    return v
}

def for_each_val(src : array<int>; blk : block<(v : int) : void>) {
    for (v in src) {
        invoke(blk, v)
    }
}

def transform_sum(src : array<int>; blk : block<(v : int) : int>) : int {
    var total = 0
    for (v in src) {
        total += invoke(blk, v)
    }
    return total
}

def chain_sum(src : array<int>; k : int; blk : block<(v : int) : int>) : int {
    // forwards the block down: the inner call inlines through on a later wave
    return transform_sum(src, blk) + k
}

def multi_exit(src : array<int>; blk : block<(v : int) : int>) : int {
    // early guard + loop tail: canonicalization synthesizes the else arm, both
    // returns are then terminal, and the splice stores them in place - no flag
    if (empty(src)) {
        return -1
    }
    var total = 0
    for (v in src) {
        total += invoke(blk, v)
    }
    return total
}

def guarded_first(src : array<int>; blk : block<(v : int) : int>) : int {
    // if/else-return pair: return canonicalization folds this to a single ternary
    // return at patch time, which makes it a valid splice callee
    if (empty(src)) {
        return -1
    }
    return invoke(blk, src[0])
}

def recursive_taker(n : int; blk : block<(v : int) : int>) : int {
    if (n <= 0) {
        return invoke(blk, 0)
    }
    return recursive_taker(n - 1, blk) + 1  // nolint:LINT010
}

def iter_sum(var src : iterator<int>; blk : block<(v : int) : int>) : int {
    // by-value-element iterator param: a generator argument is the same flavor
    var total = 0
    var v : int
    while (next(src, v)) {
        total += invoke(blk, v)
    }
    return total
}

def iter_sum_cref(var src : iterator<int const&>; blk : block<(v : int) : int>) : int {
    // element-const iterator param: an each() argument is element-mutable - a legal
    // call (variance), but the substitution would hand the optimizer a const view
    // of mutable storage (a soundness boundary, not registry debt), so the site declines
    var total = 0
    var v : int
    while (next(src, v)) {
        total += invoke(blk, v)
    }
    return total
}

struct LoHi {
    lo : int
    hi : int
}

def make_lohi(k : int; blk : block<(v : int) : int>) : LoHi {
    // struct (refType) result, copy return: splices through the manufactured
    // uninitialized result temp
    let v = invoke(blk, k)
    return LoHi(lo = v, hi = v * 10)
}

def doubled_of(src : array<int>; blk : block<(v : int) : int>) : array<int> {
    // array result, move return: the trailing `return <-` becomes a move into the temp.
    // deliberately a push loop, not a comprehension - the invoke must sit in a loop body
    var r : array<int>
    r |> reserve(length(src))
    for (v in src) {
        r |> push(invoke(blk, v))
    }
    return <- r
}

def clone_taker(n : int; blk : block<(data : array<uint8>) : void>) {
    // const block param + non-const local arg: the devirt substitution must keep the
    // param's const view (bind a const ref), or the literal's `pixels := m` - already
    // resolved to a ==const-flavored clone instance - re-types and hard-fails 30341
    var arr : array<uint8>
    arr |> resize(n)
    invoke(blk, arr)
    delete arr
}

typedef OptInt = tuple<_has : bool; _v : int>
typedef ResOptInt = tuple<_ok : bool; _v2 : OptInt; _err : string>

def inner_probe(k : int; blk : block<(v : int) : int>) : ResOptInt {
    var res : ResOptInt
    res._ok = invoke(blk, k) > 0
    res._v2._has = res._ok
    res._v2._v = k * 3
    res._err = "e"
    return <- res
}

def outer_probe(k : int; blk : block<(v : int) : int>) : OptInt {
    // the LITERAL interior call splices first, planting manufactured __inl0_* locals
    // in this body; splicing IT next must not let the rename map capture the new
    // site's synthesized result store (the interior temp has a DIFFERENT type here -
    // a captured store was error 30941 "can only move compatible type")
    var r <- inner_probe(k, $(x) => invoke(blk, x) + 1)
    if (!r._ok) {
        panic("unexpected")
    }
    return <- r._v2
}

def inner_same(k : int; blk : block<(v : int) : int>) : OptInt {
    var res : OptInt
    res._has = true
    res._v = invoke(blk, k) * 3
    return <- res
}

def outer_same(k : int; blk : block<(v : int) : int>) : OptInt {
    // same shape with MATCHING result types: a captured store compiled silently and
    // read the outer temp uninitialized - the behavioral check pins the values
    var r <- inner_same(k, $(x) => invoke(blk, x) + 1)
    if (!r._has) {
        panic("unexpected")
    }
    return <- r
}

[export]
def target_auto(src : array<int>) : int {
    return src |> transform_sum() $(v) => v * v
}

[export]
def target_void_auto(src : array<int>) : int {
    var sum = 0
    src |> for_each_val() $(v) {
        sum += v
    }
    return sum
}

[export]
def target_chain(src : array<int>) : int {
    return src |> chain_sum(100) $(v) => v + 1
}

[export]
def target_multi_exit(src : array<int>) : int {
    return src |> multi_exit() $(v) => v * 3
}

[export]
def target_canonicalized(src : array<int>) : int {
    return src |> guarded_first() $(v) => v * 3
}

[export]
def target_struct_result(k : int) : int {
    let p = make_lohi(k) $(v) => v + 1
    return p.lo + p.hi
}

[export]
def target_array_result(src : array<int>) : int {
    var doubled <- src |> doubled_of() $(v) => v * 2
    var sum = 0
    for (v in doubled) {
        sum += v
    }
    delete doubled
    return sum
}

[export]
def target_recursive(src : array<int>) : int {
    return recursive_taker(length(src)) $(v) => v + 7
}

[export]
def target_impure_args(a, b : int) : int {
    // call-order evaluation of impure arguments must survive the splice
    return transform_sum([side("a", a), side("b", b)]) $(v) => v * 2
}

[export]
def target_flavor_match(n : int) : int {
    var gen <- generator<int> {
        for (i in range(n)) {
            yield i
        }
        return false
    }
    return iter_sum(gen) $(v) => v + 1
}

[export]
def target_flavor_mismatch(src : array<int>) : int {
    return iter_sum_cref(unsafe(each(src))) $(v) => v + 1
}

[export]
def target_clone_from_param(n : int) : int {
    var pixels : array<uint8>
    clone_taker(n) $(m) {
        pixels := m
    }
    let l = length(pixels)
    delete pixels
    return l
}

[export]
def target_nested_res(k : int) : int {
    let o <- outer_probe(k) $(v) => v + 1
    return o._has ? o._v : -1
}

[export]
def target_nested_same(k : int) : int {
    let o <- outer_same(k) $(v) => v + 1
    return o._has ? o._v : -1
}

// a MUTABLE for-iterator (value type, so the by-value substitution path) reaching a
// const param through two splice levels. substituting it verbatim re-flavors every
// read inside the body, and the ==const-locked locked_note it calls stops matching
// its own instance - 30341 at a call the author never wrote
[export]
def target_const_widened_arg() : float {
    var counted : array<int>
    clear(g_probe_pool)
    clear(g_creatures)
    push(g_creatures, 1)
    push(g_creatures, 2)
    var total = 0.0
    for (creatureNode in g_creatures) {
        total += locked_forward(creatureNode, counted)
    }
    return total + float(length(counted))
}

// a sink locked to the CONST flavor: only one instance can ever exist, so a
// substitution that widens the argument's constness has no overload to fall back to
def private locked_note(var counted : array<int>; value : int const ==const) {
    push(counted, value)
}

def private counted_has(var counted : array<int>; v : int) : bool {
    var found = false
    for (c in counted) {
        if (c == v) {
            found = true
        }
    }
    return found
}

def probe_comp(v : int; blk : block<(var comp : auto(TT)? ==const) : void>) : bool {
    push(g_probe_pool, v)
    var c : int? = unsafe(addr(g_probe_pool[length(g_probe_pool) - 1]))
    if (c == null) {
        return false
    }
    invoke(blk, unsafe(reinterpret<TT?>(c)))
    return true
}

// const by-value param: the body's locked_note call is TYPED against the const view
def private locked_once(node : int; var counted : array<int>) : float {
    var m = 0.0
    if (node > 0 && !counted_has(counted, node)) {
        probe_comp(node) <| $(var comp : int?) {
            m = float(*comp)
            locked_note(counted, node)
        }
    }
    return m
}

def private locked_forward(node : int; var counted : array<int>) : float {
    var m = 0.0
    if (!counted_has(counted, node) && node > 0) {
        m = locked_once(node, counted)
    }
    return m
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
    let clean <- [for (ln in split(body, "\n")); ln; where !(ln |> has("coverage("))]
    return join(clean, "\n")
}

[test]
def test_auto_inline_fired(t : T?) {
    ast_gc_guard() {
        t |> run("block-literal call splices and devirtualizes") @(t : T?) {
            let b = body_of(t, "target_auto")
            t |> success(!has(b, "transform_sum("), "call is gone: {b}")
            t |> success(!has(b, "invoke("), "invoke devirtualized: {b}")
        }
        t |> run("void block-literal call splices") @(t : T?) {
            let b = body_of(t, "target_void_auto")
            t |> success(!has(b, "for_each_val("), "call is gone: {b}")
            t |> success(!has(b, "invoke("), "invoke devirtualized: {b}")
        }
        t |> run("forwarded block inlines through on later waves") @(t : T?) {
            let b = body_of(t, "target_chain")
            t |> success(!has(b, "chain_sum("), "outer call is gone: {b}")
            t |> success(!has(b, "transform_sum("), "forwarded call is gone: {b}")
            t |> success(!has(b, "invoke("), "invoke devirtualized: {b}")
        }
        t |> run("multi-exit callee splices with in-place stores") @(t : T?) {
            let b = body_of(t, "target_multi_exit")
            t |> success(!has(b, "multi_exit("), "call is gone: {b}")
            t |> success(!has(b, "_ret"), "terminal returns need no flag: {b}")
        }
        t |> run("if/else-return callee canonicalizes and splices") @(t : T?) {
            let b = body_of(t, "target_canonicalized")
            t |> success(!has(b, "guarded_first("), "call is gone: {b}")
        }
        t |> run("struct-result callee splices through the result temp") @(t : T?) {
            let b = body_of(t, "target_struct_result")
            t |> success(!has(b, "make_lohi("), "call is gone: {b}")
        }
        t |> run("array-result callee splices, move return preserved") @(t : T?) {
            let b = body_of(t, "target_array_result")
            t |> success(!has(b, "doubled_of("), "call is gone: {b}")
            t |> success(!has(b, "invoke("), "invoke devirtualized: {b}")
        }
        t |> run("recursive callee declines") @(t : T?) {
            let b = body_of(t, "target_recursive")
            t |> success(has(b, "recursive_taker("), "call stays regular: {b}")
        }
        t |> run("matching element flavor splices") @(t : T?) {
            let b = body_of(t, "target_flavor_match")
            t |> success(!has(b, "iter_sum("), "call is gone: {b}")
        }
        t |> run("element flavor change declines") @(t : T?) {
            let b = body_of(t, "target_flavor_mismatch")
            t |> success(has(b, "iter_sum_cref("), "call stays regular: {b}")
        }
        t |> run("const-view-preserving substitution keeps := splice-safe") @(t : T?) {
            let b = body_of(t, "target_clone_from_param")
            t |> success(!has(b, "clone_taker("), "call is gone: {b}")
        }
        t |> run("const-widened by-value argument binds a temp, not a substitution") @(t : T?) {
            let b = body_of(t, "target_const_widened_arg")
            t |> success(!has(b, "locked_forward("), "outer call is gone: {b}")
            t |> success(!has(b, "locked_once("), "interior call is gone: {b}")
            // the mutable iterator must NOT reach the spliced body verbatim - it binds
            // a const temp, which is what keeps the locked instance matching
            t |> success(!has(b, "locked_note(counted,creatureNode)"),
                "mutable iterator did not substitute verbatim: {b}")
        }
        t |> run("callee with interior splice splices again without capture") @(t : T?) {
            let b = body_of(t, "target_nested_res")
            t |> success(!has(b, "outer_probe("), "outer call is gone: {b}")
            t |> success(!has(b, "inner_probe("), "interior call is gone: {b}")
            let b2 = body_of(t, "target_nested_same")
            t |> success(!has(b2, "outer_same("), "outer call is gone: {b2}")
            t |> success(!has(b2, "inner_same("), "interior call is gone: {b2}")
        }
    }
}

[test]
def test_auto_inline_behavior(t : T?) {
    t |> run("iterator argument borrowing a temporary keeps call-scoped storage alive") @(t : T?) {
        // devirt/auto splices decline composed iterator arguments: an arg temp would
        // shrink the borrowed temporary's lifetime to the temp's own statement and
        // the spliced body would iterate freed memory (latent in v1's devirt too)
        let r <- invoke($(var it : iterator<int&>) : array<int> {
            var buf : array<int>
            for (x in it) {
                buf |> push(x * 2)
            }
            return <- buf
        }, unsafe(each([5, 3, 8, 1])))
        t |> equal(4, length(r))
        t |> equal(10, r[0])
        t |> equal(2, r[3])
    }
    t |> run("values preserved") @(t : T?) {
        let src <- [1, 2, 3, 4]
        t |> equal(target_auto(src), 30)
        t |> equal(target_void_auto(src), 10)
        t |> equal(target_chain(src), 114)
        t |> equal(target_multi_exit(src), 30)
        t |> equal(target_canonicalized(src), 3)
        t |> equal(target_struct_result(4), 55)
        t |> equal(target_array_result(src), 20)
        let none : array<int>
        t |> equal(target_multi_exit(none), -1)
        t |> equal(target_canonicalized(none), -1)
        t |> equal(target_array_result(none), 0)
        t |> equal(target_recursive(src), 11)
        t |> equal(target_flavor_match(4), 10)
        t |> equal(target_flavor_mismatch(src), 14)
        t |> equal(target_nested_res(5), 15)
        t |> equal(target_nested_same(4), 18)
        t |> equal(target_clone_from_param(4), 4)
        t |> equal(target_const_widened_arg(), 5.0)
    }
    t |> run("impure arguments evaluate once in call order") @(t : T?) {
        g_log = ""
        t |> equal(target_impure_args(3, 4), 14)
        t |> equal(g_log, "ab")
    }
}
