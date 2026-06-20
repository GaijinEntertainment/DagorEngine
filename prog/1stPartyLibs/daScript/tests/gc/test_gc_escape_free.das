// Escape-analysis static free - SOUNDNESS. A local pointer allocated via `new` whose every use
// is a field access never escapes its scope, so it is freed at scope exit. The load-bearing
// property here: a pointer that DOES escape (returned, stored, aliased) must NOT be freed.
// Each test ends with heap_collect(validate=true), which throws if a still-reachable object was
// wrongly freed. (heap_collect needs `options gc`; see test_gc_escape_free_frees.das for the
// proof that the freeing itself happens without any GC.)
options gen2
options persistent_heap
options gc
options no_aot
options force_escape_free = true

require dastest/testing_boost public
require UnitTest

struct Node {
    x : int
    y : int
}

// nested ownership: scope-free reclaims the Outer slot + its owned arrays, but NOT the `child`
// pointer field (GcPod never chases pointers - they might be shared). The orphaned child is left
// to the GC, exactly as if Outer had become garbage. This test verifies that interaction is sound.
struct Inner { vals : array<int> }
struct Outer { tag : int; child : Inner ? }

def churn_nested() : int {
    var acc = 0
    for (i in range(500)) {
        var p = new Outer(tag = i, child = new Inner(vals <- [for (x in range(10)); x]))
        acc += p.tag + length(p.child.vals)
    }
    return acc
}

var g_kept : Node?
var g_list : array<Node?>

// the heap GC reclaims memory WITHOUT running finalizers; the scope-free optimization does the
// same (GC-style raw free, never `delete`). So an object with a USER finalizer is still freed,
// but its finalizer must NOT fire - exactly as the GC would leave it.
var g_user_fin_calls = 0
struct WithFinalizer { tag : int }
def finalize(var _self : WithFinalizer) { g_user_fin_calls ++ }

def churn_user_finalizer() {
    for (i in range(100)) {
        var p = new WithFinalizer(tag = i)   // freed via raw collect; finalizer must not run
        let _t = p.tag
    }
}

def leak_into_global(i : int) {
    var p = new Node(x = i, y = i)
    g_kept = p
}

// `p` is captured by a lambda stored in a global -> p escapes via the capture, must NOT be freed
var g_lam : lambda<() : int>
def capture_into_global(i : int) {
    var p = new Node(x = i, y = i)
    g_lam <- @() : int { return p.x }
}

def make_node(i : int) : Node? {
    var p = new Node(x = i, y = i)
    return p
}

// `p` passed to an impure (modifyExternal) bound C++ function that retains it -> must NOT be freed
def pass_to_retaining(i : int) {
    var p = new Node(x = i, y = i)
    test_escape_keep(p)
}

def push_into_global(i : int) {
    var p = new Node(x = i, y = i)
    g_list |> push(p)
}

def alias_then_escape(i : int) {
    var p = new Node(x = i, y = i)
    var q = p
    g_kept = q
}

[test]
def test_escape_free_does_not_free_global(t : T?) {
    leak_into_global(42)
    unsafe { heap_collect(true, true) }
    t |> equal(g_kept.x, 42)
}

[test]
def test_escape_free_does_not_free_returned(t : T?) {
    var n = make_node(7)
    unsafe { heap_collect(true, true) }
    t |> equal(n.x, 7)
    unsafe { delete n }
}

[test]
def test_escape_free_does_not_free_pushed(t : T?) {
    g_list |> clear()
    push_into_global(11)
    push_into_global(22)
    unsafe { heap_collect(true, true) }
    t |> equal(length(g_list), 2)
    t |> equal(g_list[0].x, 11)
    t |> equal(g_list[1].x, 22)
}

[test]
def test_escape_free_does_not_free_aliased(t : T?) {
    alias_then_escape(99)
    unsafe { heap_collect(true, true) }
    t |> equal(g_kept.x, 99)    // survived: neither p nor its alias q was statically freed
}

// nested ownership: Outer is scope-freed each iteration, its Inner child is orphaned and reclaimed
// by the GC. The validating heap_collect proves no dangling pointer results from the partial free.
[test]
def test_escape_free_nested_sound(t : T?) {
    let s = churn_nested()
    unsafe { heap_collect(true, true) }
    t |> equal(s, (500 - 1) * 500 / 2 + 500 * 10)
}

// a local captured by a lambda escapes through the capture frame; freeing it would dangle the
// lambda's captured pointer. It must survive.
[test]
def test_escape_free_does_not_free_captured(t : T?) {
    capture_into_global(55)
    unsafe { heap_collect(true, true) }
    t |> equal(invoke(g_lam), 55)
}

// a pointer passed to an impure (retaining) bound C++ function must NOT be freed. After retaining
// it, we churn freeable allocations that would reuse its slot if it had been wrongly freed, then
// confirm the retained value is intact.
[test]
def test_escape_free_does_not_free_retained(t : T?) {
    pass_to_retaining(42)
    for (_i in range(1000)) {
        var z = new Node(x = -1, y = -1)
        let _ = test_escape_pure(z)     // freeable -> reuses slots
    }
    t |> equal(test_escape_retained_first_int(), 42)   // retained pointer survived -> not freed
}

// GC-equivalence: the optimization frees via a raw collect (no `delete`), so even a type with a
// user finalizer is reclaimed WITHOUT calling that finalizer - exactly as the GC would.
[test]
def test_escape_free_does_not_run_user_finalizer(t : T?) {
    g_user_fin_calls = 0
    churn_user_finalizer()
    t |> equal(g_user_fin_calls, 0)   // freed without finalizing -> matches GC behavior
}
