// Lattice escape analysis - SPEC tests for the unified alias-set + return-ownership model.
//
// Methodology (mirrors test_gc_escape_free_frees.das):
//   * no `options gc` + `options no_optimization`: the escape-free static delete is the ONLY thing
//     that can keep the heap flat across a non-escaping allocation loop (nothing else reclaims the
//     garbage, and the allocation can't be elided). A control loop that escapes proves that.
//   * `options persistent_heap` is REQUIRED, not cosmetic. The shared / non-stack-relocatable cases
//     (aliased locals, call-result owners, block-captured) are heap-allocated and freed one-by-one
//     via builtin_scope_free. Only the free-list persistent heap reclaims+reuses an individually
//     freed object REGARDLESS of allocation order; the default bump heap only reclaims LIFO, so once
//     the test harness interleaves other allocations a genuinely-freed loop still shows O(N) growth
//     and the assertion false-fails. (Drop persistent_heap and the alias/return/block freed-tests go
//     red, while the unique-local tests still pass - those are stack-allocated and never hit the heap.)
//
// The "freed" assertion is exactly `grew == 0`: a fully-freeing loop reclaims every slot it allocates
// (free-list reuse), so the heap high-water does not move at all - verified to hold in-harness, not
// just in isolation. A leaked loop instead grows by O(N) (N=2000). Value/churn cases instead read a
// supposedly-live object back after reusing slots, so a wrongly-freed pointee surfaces as a corrupt
// result. [NEW] marks behavior the unified lattice added (shared-but-freeable aliases; caller frees a
// fresh-owned return value) - now implemented and passing.
options gen2
options persistent_heap
options no_aot
options no_optimization
options force_escape_free = true
options force_partial_escape_free = true   // flow-sensitive partial frees (default off) - this suite tests them

require dastest/testing_boost public
require UnitTest

let N = 2000

struct Node {
    x : int
    y : int
}

var g_list : array<Node?>
var g_lam : lambda<() : int>

// [NEW] alias within the function, neither escapes: `var q = p` makes p SHARED (alias set
// {p,q}) - still freeable at scope exit, just not stack-relocatable. The old analysis
// treated any copy as an escape and leaked it.
def sum_alias_local() : int {
    var total = 0
    for (i in range(N)) {
        var p = new Node(x = i, y = i * 2)
        var q = p
        total += q.x + p.y
    }
    return total
}

// [NEW] return ownership: the callee allocates and returns it (FreshOwned), so the CALLER
// owns `x` and frees it at scope exit - even though x's init is a call, not a `new`.
def make_node(i : int) : Node? {
    return new Node(x = i, y = i * 2)
}
def sum_return_owned() : int {
    var total = 0
    for (i in range(N)) {
        var x = make_node(i)
        total += x.x + x.y
    }
    return total
}

// [NEW] transitive return ownership: make_node_t only forwards the fresh result, so the
// FreshOwned provenance transits the call chain.
def make_node_t(i : int) : Node? => make_node(i)
def sum_return_owned_transitive() : int {
    var total = 0
    for (i in range(N)) {
        var x = make_node_t(i)
        total += x.x + x.y
    }
    return total
}

// control: stored into a global -> escapes -> heap MUST grow (proves allocs are not elided)
def sum_escaping() : int {
    var total = 0
    g_list |> reserve(length(g_list) + N)
    for (i in range(N)) {
        var p = new Node(x = i, y = i * 2)
        g_list |> push(p)
        total += p.x + p.y
    }
    return total
}

// param-alias return: passthru hands its parameter straight back, so the caller's result
// `c` ALIASES `owned` and must NOT be freed as a second owner (that would double-free).
def passthru(var p : Node?) : Node? => p
def use_param_alias(i : int) : int {
    var owned = new Node(x = i, y = i * 2)
    var c = passthru(owned)
    return c.x + owned.y    // both read the SAME object
}

[test]
def test_alias_local_freed(t : T?) {            // [NEW] shared local is still freed
    let before = heap_bytes_allocated()
    let s = sum_alias_local()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, 3 * ((N - 1) * N / 2))
    t |> equal(grew, 0ul)
}

[test]
def test_return_ownership_freed(t : T?) {       // [NEW] caller frees the fresh-owned result
    let before = heap_bytes_allocated()
    let s = sum_return_owned()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, 3 * ((N - 1) * N / 2))
    t |> equal(grew, 0ul)
}

[test]
def test_return_ownership_transitive(t : T?) {  // [NEW] provenance transits the call chain
    let before = heap_bytes_allocated()
    let s = sum_return_owned_transitive()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, 3 * ((N - 1) * N / 2))
    t |> equal(grew, 0ul)
}

[test]
def test_escaping_control_grows(t : T?) {       // control: escaping allocations are retained
    g_list |> clear()
    let before = heap_bytes_allocated()
    let s = sum_escaping()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, 3 * ((N - 1) * N / 2))
    t |> equal(length(g_list), N)
    t |> success(grew >= uint64(N) * (typeinfo sizeof(type<Node>)))
}

[test]
def test_param_alias_no_double_free(t : T?) {   // [NEW] param-alias result is not double-freed
    var acc = 0
    for (i in range(N)) {
        acc += use_param_alias(i)
    }
    t |> equal(acc, 3 * ((N - 1) * N / 2))
}

// ===== Lambda capture: a pointer captured into a frame escapes through that frame =====
// All SOUNDNESS guards (must hold before AND after the lattice work): a captured pointer
// is reachable from the frame, so it must NOT be statically freed. A wrongly-freed pointee
// would be corrupted when the churn loop reuses its slot, or dangle in the surviving frame.

// captured into a GLOBAL lambda -> escapes via the global -> must NOT be freed
def capture_into_global_lambda(i : int) {
    var p = new Node(x = i, y = i * 2)
    g_lam <- @() : int { return p.x + p.y }
}

// captured into a LOCAL lambda, invoked and deleted in the same scope
def read_via_local_lambda(i : int) : int {
    var p = new Node(x = i, y = i * 2)
    var f = @() : int { return p.x + p.y }
    let r = invoke(f)
    unsafe { delete f }
    return r
}

// a lambda capturing p is RETURNED -> p escapes through the returned frame
def make_reader(i : int) : lambda<() : int> {
    var p = new Node(x = i, y = i * 2)
    return <- @() : int { return p.x + p.y }
}

[test]
def test_capture_into_global_lambda_not_freed(t : T?) {
    capture_into_global_lambda(7)
    var churn = 0
    for (j in range(N)) {                       // freeable allocs reuse any wrongly-freed slot
        var z = new Node(x = j, y = j)
        churn += z.x
    }
    t |> equal(churn, (N - 1) * N / 2)
    t |> equal(invoke(g_lam), 21)               // 7 + 14 - captured p survived
    unsafe { delete g_lam }
}

[test]
def test_capture_into_local_lambda_sound(t : T?) {
    var acc = 0
    for (i in range(N)) {
        acc += read_via_local_lambda(i)
    }
    t |> equal(acc, 3 * ((N - 1) * N / 2))
}

[test]
def test_returned_lambda_capture_not_freed(t : T?) {
    var acc = 0
    for (i in range(N)) {
        var f = make_reader(i)
        acc += invoke(f)
        unsafe { delete f }
    }
    t |> equal(acc, 3 * ((N - 1) * N / 2))
}

// a GENERATOR capturing a fresh local is RETURNED: the captured pointer escapes through the
// generator frame (outlives the function) -> must not be freed at scope exit
def make_counter(i : int) : iterator<int> {
    var p = new Node(x = i, y = i * 2)
    return <- generator<int> {
        yield p.x
        yield p.y
        return false
    }
}

[test]
def test_returned_generator_capture_not_freed(t : T?) {
    var acc = 0
    for (i in range(N)) {
        for (v in make_counter(i)) { acc += v }   // yields p.x + p.y == 3*i; corrupt if p was freed
    }
    t |> equal(acc, 3 * ((N - 1) * N / 2))
}

// ===== Block capture: a block cannot be stored/returned, so its captures do not escape =====
// A block body is walked inline in the enclosing function, so a pointer used only as a field
// base inside a block stays freeable; a pointer leaked to a global INSIDE a block still escapes.

def run(blk : block<() : void>) {
    invoke(blk)
}

// p used only via field access inside a block -> does not escape -> freed at scope exit
def sum_block_field(i : int) : int {
    var p = new Node(x = i, y = i * 2)
    var total = 0
    run() {
        total += p.x + p.y
    }
    return total
}

// p leaked to a global from inside a block -> escapes -> must NOT be freed
def block_escapes(i : int) {
    var p = new Node(x = i, y = i * 2)
    run() {
        g_list |> push(p)
    }
}

[test]
def test_block_field_use_freed(t : T?) {        // block capture does not block the free
    let before = heap_bytes_allocated()
    var acc = 0
    for (i in range(N)) {
        acc += sum_block_field(i)
    }
    let grew = heap_bytes_allocated() - before
    t |> equal(acc, 3 * ((N - 1) * N / 2))
    t |> equal(grew, 0ul)
}

[test]
def test_block_escape_grows(t : T?) {           // escape from inside a block still escapes
    g_list |> clear()
    let before = heap_bytes_allocated()
    for (i in range(N)) {
        block_escapes(i)
    }
    let grew = heap_bytes_allocated() - before
    t |> equal(length(g_list), N)
    t |> success(grew >= uint64(N) * (typeinfo sizeof(type<Node>)))
}

// ===== Adversarial: try to fool the analysis (aliasing, reassignment, conditional escape,
// return-param-alias, unsafe, self-reference). All are SOUNDNESS guards unless tagged precision. =====

var g_anchor : Node?

// multi-hop alias chain ending in a global store: NONE of a/b/c may be freed
def alias_chain_to_global(i : int) {
    var a = new Node(x = i, y = i * 2)
    var b = a
    var c = b
    g_list |> push(c)
}

// flow-insensitive: p escapes on SOME iterations -> must be treated as escaping on all
def conditional_push(i : int) : int {
    var p = new Node(x = i, y = i * 2)
    if (i % 3 == 0) { g_list |> push(p) }
    return p.x + p.y
}

// p is a fresh alloc but then REASSIGNED to alias the global: freeing p at scope exit would
// free the anchor (double free). The reassignment (write to p) must mark it escaping.
def maybe_reassign(i : int) : int {
    var p = new Node(x = i, y = i * 2)
    if (i % 2 == 0) { p = g_anchor }
    return p.x
}

// one sibling escapes, the other does not: keep retained, drop freed (precision + soundness).
// keep escapes to a single global (no array growth), so total heap growth cleanly separates
// "keeps leak ~N, drops freed ~0" from the unsound "both retained ~2N".
def sibling_mixed(i : int) : int {
    var keep = new Node(x = i, y = i * 2)
    var drop = new Node(x = i + 1, y = i + 2)
    g_anchor = keep
    return drop.x + drop.y
}

// callee returns its parameter; the caller's result aliases the caller's own fresh local,
// so neither may be freed (would double-free the shared object)
def ret_param(var a : Node?) : Node? { var x = a; return x }
def use_ret_param(i : int) : int {
    var m = new Node(x = i, y = i * 2)
    var c = ret_param(m)
    return c.x + m.y
}

// a function containing `unsafe` is excluded from analysis (hidden aliasing via addr): its
// local must never be statically freed
def unsafe_addr_local(i : int) : int {
    var p = new Node(x = i, y = i * 2)
    var r = p.x + p.y
    unsafe {
        let pp = addr(p)               // taking the address marks the function unsafe -> excluded
        if (pp == null) { r = -1 }     // never taken; consumes pp without a dead store
    }
    return r
}

[test]
def test_alias_chain_escape_sound(t : T?) {
    g_list |> clear()
    let before = heap_bytes_allocated()
    for (i in range(N)) { alias_chain_to_global(i) }
    let grew = heap_bytes_allocated() - before
    t |> equal(length(g_list), N)
    var ok = true
    for (n in g_list) { if (n.y != n.x * 2) { ok = false } }
    t |> success(ok, "3-hop aliased objects survived intact")
    t |> success(grew >= uint64(N) * (typeinfo sizeof(type<Node>)))
}

[test]
def test_conditional_escape_sound(t : T?) {
    g_list |> clear()
    var acc = 0
    for (i in range(N)) { acc += conditional_push(i) }
    t |> equal(acc, 3 * ((N - 1) * N / 2))
    var ok = true
    for (n in g_list) { if (n.x % 3 != 0 || n.y != n.x * 2) { ok = false } }
    t |> success(ok, "conditionally-escaped objects intact")
}

[test]
def test_reassigned_no_double_free(t : T?) {
    g_anchor = new Node(x = 777, y = 1)
    var acc = 0
    for (i in range(N)) { acc += maybe_reassign(i) }
    t |> equal(g_anchor.x, 777)
}

[test]
def test_sibling_mixed(t : T?) {
    // soundness: the escaping sibling (keep) must survive intact while the non-escaping sibling
    // (drop) is freed. (drop's freeing is verified precisely by test_alias_local_freed / sum_multi;
    // here keeps necessarily accumulate, so total heap can't isolate it.)
    var acc = 0
    for (i in range(N)) { acc += sibling_mixed(i) }
    t |> equal(acc, N * N + 2 * N)              // sum of (i+1)+(i+2) over [0,N)
    t |> equal(g_anchor.x, N - 1)               // last keep retained intact (not clobbered by a freed drop)
}

[test]
def test_return_param_alias_no_double_free(t : T?) {
    var acc = 0
    for (i in range(N)) { acc += use_ret_param(i) }
    t |> equal(acc, 3 * ((N - 1) * N / 2))
}

[test]
def test_unsafe_local_not_freed(t : T?) {
    var acc = 0
    for (i in range(N)) { acc += unsafe_addr_local(i) }
    t |> equal(acc, 3 * ((N - 1) * N / 2))
}

// ===== Adversarial closures: capture combined with aliasing and nesting =====

// a lambda captures an ALIAS of a fresh local, then escapes to a global: aliasing + capture
// together must still keep the object alive
def capture_alias_into_global(i : int) {
    var p = new Node(x = i, y = i * 2)
    var q = p
    g_lam <- @() : int { return q.x + q.y }
}

// a block aliases a fresh local and leaks the alias to a global from inside the block
def block_aliases_escape(i : int) {
    var p = new Node(x = i, y = i * 2)
    run() {
        var q = p
        g_list |> push(q)
    }
}

// a lambda is created INSIDE a block, captures the outer local, and escapes to a global
def lambda_in_block(i : int) {
    var p = new Node(x = i, y = i * 2)
    run() {
        g_lam <- @() : int { return p.x }
    }
}

[test]
def test_lambda_captures_alias_sound(t : T?) {
    capture_alias_into_global(11)
    var churn = 0
    for (j in range(N)) { var z = new Node(x = j, y = j); churn += z.x }   // freeable -> reuse slots
    t |> equal(churn, (N - 1) * N / 2)
    t |> equal(invoke(g_lam), 33)        // 11 + 22; captured alias survived the churn
    unsafe { delete g_lam }
}

[test]
def test_block_aliases_escape_grows(t : T?) {
    g_list |> clear()
    let before = heap_bytes_allocated()
    for (i in range(N)) { block_aliases_escape(i) }
    let grew = heap_bytes_allocated() - before
    t |> equal(length(g_list), N)
    var ok = true
    for (n in g_list) { if (n.y != n.x * 2) { ok = false } }
    t |> success(ok, "block-aliased escaped objects intact")
    t |> success(grew >= uint64(N) * (typeinfo sizeof(type<Node>)))
}

[test]
def test_lambda_in_block_sound(t : T?) {
    lambda_in_block(13)
    var churn = 0
    for (j in range(N)) { var z = new Node(x = -j, y = j); churn += z.y }
    t |> equal(churn, (N - 1) * N / 2)
    t |> equal(invoke(g_lam), 13)        // captured local survived despite nesting
    unsafe { delete g_lam }
}

// a pointer used only inside string interpolation "{p}" is formatted by a read-only walk (a fresh
// string is built, the pointer is not retained), so it does not escape -> freed at scope exit
def format_only(i : int) : int {
    var p = new Node(x = i, y = i * 2)
    feint("{p}")
    return p.x + p.y
}

[test]
def test_string_builder_element_freed(t : T?) {
    let before = heap_bytes_allocated()
    var acc = 0
    for (i in range(N)) { acc += format_only(i) }
    let grew = heap_bytes_allocated() - before
    t |> equal(acc, 3 * ((N - 1) * N / 2))
    t |> equal(grew, 0ul)        // formatted-only local is freed (string builder does not retain it)
}

// ===== [PEA] flow-sensitive partial free =====
// The object escapes on ONE branch (stored to a global + returned) but not on the other. The
// flow-INsensitive verdict is "escapes" -> the scope-exit pass leaves it to GC. The flow-sensitive
// pass (CFG + escape flow + liveness) frees it on the branch where it is dead and has not escaped.
var g_pea : array<Node?>

def pea_partial(keep : bool, i : int) : Node? {
    var p = new Node(x = i, y = i * 2)
    if (keep) {
        g_pea |> push(p)         // escapes via the global on this path
        return p
    } else {
        return null              // p is dead and unescaped here -> PEA frees it
    }
}

def pea_sum_freed_path() : int {
    var seen = 0
    for (i in range(N)) {
        var r = pea_partial(false, i)   // non-escaping branch every iteration -> p freed each time
        if (r != null) { seen ++ }
    }
    return seen
}

[test]
def test_pea_partial_free_reclaims(t : T?) {        // [PEA] free on the non-escaping branch reclaims the heap
    g_pea |> clear()
    let before = heap_bytes_allocated()
    let seen = pea_sum_freed_path()
    let grew = heap_bytes_allocated() - before
    t |> equal(seen, 0)                 // every call took the null branch
    t |> equal(length(g_pea), 0)        // nothing escaped
    t |> equal(grew, 0ul)               // freed each iteration, free-list reuses the slot
}

[test]
def test_pea_partial_escape_path_ok(t : T?) {       // [PEA] escaping branch keeps the object live (no over-free)
    g_pea |> clear()
    let r = pea_partial(true, 5)
    t |> equal(length(g_pea), 1)        // escaped via the global
    t |> equal(r.x, 5)                  // not freed on this path -> value intact
    t |> equal(r.y, 10)
    g_pea |> clear()
}

// [PEA] within-block death: the object is USED on the non-escaping branch (read), then dies before the
// branch returns. The free goes at the branch's scope exit - after the last use - not at its entry.
def pea_use_then_free(keep : bool, i : int) : int {
    var p = new Node(x = i, y = i * 2)
    if (keep) {
        g_pea |> push(p)         // escapes via the global on this path
        return p.x
    } else {
        let s = p.x + p.y        // p is USED here, then dead -> freed at the branch's scope exit
        return s
    }
}

def pea_sum_use_then_free() : int {
    var total = 0
    for (i in range(N)) {
        total += pea_use_then_free(false, i)   // non-escaping branch: p read then freed each iteration
    }
    return total
}

[test]
def test_pea_within_block_death_freed(t : T?) {     // [PEA] free after the last use on the non-escaping branch
    g_pea |> clear()
    let before = heap_bytes_allocated()
    let s = pea_sum_use_then_free()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, 3 * ((N - 1) * N / 2))    // every read returned the right value before the free
    t |> equal(length(g_pea), 0)
    t |> equal(grew, 0ul)                   // freed after use each iteration -> heap flat
}

// [PEA] early-return escape: the escaping arm RETURNS, so the continuation after the `if` is reached
// ONLY on the non-escaping fall-through, where p is dead-at-entry. PEA frees p as a statement before
// the continuation (the contBefore insertion path, distinct from a scope-exit finalList).
def pea_early_return(keep : bool, i : int) : int {
    var p = new Node(x = i, y = i * 2)
    if (keep) {
        g_pea |> push(p)
        return p.x
    }
    let after = i * 3                        // continuation anchor: the free is inserted before this
    return after
}
[test]
def test_pea_early_return_freed(t : T?) {
    g_pea |> clear()
    let before = heap_bytes_allocated()
    var s = 0
    for (i in range(N)) { s += pea_early_return(false, i) }   // always the non-escaping path
    let grew = heap_bytes_allocated() - before
    t |> equal(s, 3 * ((N - 1) * N / 2))
    t |> equal(length(g_pea), 0)
    t |> equal(grew, 0ul)                   // freed before the continuation statement -> heap flat
}
[test]
def test_pea_early_return_escape_survives(t : T?) {
    g_pea |> clear()
    var acc = 0
    for (i in range(8)) { acc += pea_early_return(true, i) }  // escaping arm: object must survive
    t |> equal(length(g_pea), 8)
    var ok = true
    for (n in g_pea) { if (n.y != n.x * 2) { ok = false } }
    t |> success(ok, "early-return escaped objects intact")
    g_pea |> clear()
}

// [PEA] nested branches: p escapes only on the deepest path (a && b). On a && !b and on !a it is dead,
// freed at two different continuation levels. Driven down !a -> always the freed path.
def pea_nested(a : bool, b : bool, i : int) : int {
    var p = new Node(x = i, y = i * 2)
    if (a) {
        if (b) {
            g_pea |> push(p)
            return p.x
        }
        return i + 1                        // a && !b: p dead
    }
    return i + 2                            // !a: p dead
}
[test]
def test_pea_nested_freed(t : T?) {
    g_pea |> clear()
    let before = heap_bytes_allocated()
    var s = 0
    for (i in range(N)) { s += pea_nested(false, false, i) }
    let grew = heap_bytes_allocated() - before
    t |> equal(s, (N - 1) * N / 2 + 2 * N)
    t |> equal(grew, 0ul)
}

// [PEA] three tracked objects, divergent fates in one frame: `a` conditionally escapes (escaping arm
// returns early, so PEA frees it on the fall-through), `b` and `c` never escape (stack-allocated).
// Exercises the per-object free-site map alongside multiple live stack allocations in one scope.
def pea_multi(keep_a : bool, i : int) : int {
    var a = new Node(x = i, y = i * 2)
    var b = new Node(x = i + 1, y = i + 1)
    var c = new Node(x = i + 2, y = i + 2)
    if (keep_a) { g_pea |> push(a); return -1 }
    return b.x + c.x
}
[test]
def test_pea_multi_freed(t : T?) {
    g_pea |> clear()
    let before = heap_bytes_allocated()
    var s = 0
    for (i in range(N)) { s += pea_multi(false, i) }
    let grew = heap_bytes_allocated() - before
    t |> equal(s, (N - 1) * N + 3 * N)      // sum of (i+1)+(i+2)
    t |> equal(grew, 0ul)
}

var g_clob : array<Node?>
def pea_for_source(keep : bool, i : int) : int {
    var p = new Node(x = i + 3, y = 0)
    if (keep) { g_pea |> push(p); return p.x }
    var clobber = new Node(x = 999, y = 0)
    g_clob |> push(clobber)
    var sum = 0
    for (e in range(p.x)) { sum += e }
    return sum
}
[test]
def test_pea_for_source_not_freed(t : T?) {
    g_pea |> clear(); g_clob |> clear()
    var acc = 0
    for (i in range(10)) { acc += pea_for_source(false, i) }
    t |> equal(acc, 285)
    g_pea |> clear(); g_clob |> clear()
}
