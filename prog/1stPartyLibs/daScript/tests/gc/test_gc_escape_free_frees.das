// Escape-analysis static free - PROOF THAT THE OPTIMIZATION ACTUALLY FREES.
//
// To attribute the freeing to escape analysis (and not to GC or to the optimizer eliding a dead
// allocation), this file deliberately:
//   * omits `options gc`            -> no auto-collection can reclaim the garbage
//   * sets `options no_optimization` -> the optimizer can't drop the otherwise-dead allocations
// so the ONLY thing that can keep the heap flat across a non-escaping allocation loop is the
// escape-free static delete.
//
// test_escape_free_actually_frees : non-escaping loop -> heap stays flat (the delete ran).
// test_escape_free_control_grows  : the SAME loop, escaping -> heap grows by N nodes. This is the
//   control: if it did NOT grow, allocations would be elided and the test above would be vacuous.
options gen2
options persistent_heap
options no_aot
options no_optimization
options force_escape_free = true

require dastest/testing_boost public
require UnitTest

let N = 2000

struct Node {
    x : int
    y : int
}

// owns a heap field: the raw collect frees `data`'s backing AND the struct slot
struct OwningNode {
    tag : int
    data : array<int>
}

var g_list : array<Node?>

// every use of `p` is a field access -> p does not escape -> freed at scope exit
def sum_nonescaping() : int {
    var total = 0
    for (i in range(N)) {
        var p = new Node(x = i, y = i * 2)
        total += p.x + p.y
    }
    return total
}

// multi-variable let: each variable gets its OWN fresh allocation, so both are freed (no alias)
def sum_multi() : int {
    var total = 0
    for (i in range(N)) {
        var a, b : Node? = new Node(x = i, y = i * 2)
        total += a.x + b.y
    }
    return total
}

// the free still runs in a function with try/recover (on the normal path)
def sum_with_try() : int {
    var total = 0
    for (i in range(N)) {
        var p = new Node(x = i, y = i * 2)
        try { total += p.x + p.y } recover { }
    }
    return total
}

// passed only to a pure built-in (test_escape_pure: no side effects, scalar return) -> freed
def sum_pure_builtin() : int {
    var total = 0
    for (i in range(N)) {
        var p = new Node(x = i, y = i * 2)
        total += test_escape_pure(p)
    }
    return total
}

// non-escaping owner of a heap field; the raw collect frees the owned array too
def sum_owning() : int {
    var total = 0
    for (i in range(N)) {
        var p = new OwningNode(tag = i, data <- [for (x in range(20)); x])
        total += p.tag + length(p.data)
    }
    return total
}

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

[test]
def test_escape_free_actually_frees(t : T?) {
    let before = heap_bytes_allocated()
    let s = sum_nonescaping()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, 3 * ((N - 1) * N / 2))
    t |> success(grew < uint64(20 * (typeinfo sizeof(type<Node>))))   // a few slots, not N nodes
}

// multi-variable let: both a and b are independently owned and freed -> heap stays flat (no alias,
// no double-free).
[test]
def test_escape_free_multi_var(t : T?) {
    let before = heap_bytes_allocated()
    let s = sum_multi()
    let grew = heap_bytes_allocated() - before
    t |> success(grew < uint64(40 * (typeinfo sizeof(type<Node>))))
}

// passed only to a pure (SideEffects::none) bound C++ function -> can't escape -> freed
[test]
def test_escape_free_pure_call(t : T?) {
    let before = heap_bytes_allocated()
    let s = sum_pure_builtin()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, 0)
    t |> success(grew < uint64(40 * (typeinfo sizeof(type<Node>))))
}

// the optimization still frees in a function containing try/recover (normal path).
[test]
def test_escape_free_under_try_recover(t : T?) {
    let before = heap_bytes_allocated()
    let s = sum_with_try()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, 3 * ((N - 1) * N / 2))
    t |> success(grew < uint64(40 * (typeinfo sizeof(type<Node>))))
}

// the raw collect must free the owned heap field too, not just the struct slot. If the array
// backing were left, the N `data` arrays (~N*20*4 bytes) would leak and the heap would grow.
[test]
def test_escape_free_collects_owned_heap(t : T?) {
    let before = heap_bytes_allocated()
    let s = sum_owning()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, (N - 1) * N / 2 + N * 20)
    t |> success(grew < uint64(40 * (typeinfo sizeof(type<OwningNode>))))   // owned arrays freed too
}

[test]
def test_escape_free_control_grows(t : T?) {
    g_list |> clear()
    let before = heap_bytes_allocated()
    let s = sum_escaping()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, 3 * ((N - 1) * N / 2))
    t |> equal(length(g_list), N)
    t |> success(grew >= uint64(N) * (typeinfo sizeof(type<Node>)))   // N live nodes retained
}
