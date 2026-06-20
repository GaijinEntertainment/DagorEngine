// Escape-analysis stack allocation - PROOF THAT NON-ESCAPING `new` NEVER TOUCHES THE HEAP.
//
// With `options force_allocate_on_stack`, a local pointer allocated via `new` whose every use is a
// field access does not escape, so its pointee is placed in the stack frame instead of the heap.
// To attribute the flat heap to stack allocation (and not to GC or dead-allocation elision) this
// file omits `options gc` and sets `options no_optimization`. Only stack allocation can keep the
// heap EXACTLY flat across a non-escaping allocation loop.
//
// Covers both lowerings of `new`:
//   * `new Node()`            -> ExprNew (no initializer) -> SimNode_NewStack
//   * `new Node(x = .., ..)`  -> ExprAscend over a make-local -> in-frame build, no heap copy
//
// force_allocate_on_stack alone (no force_escape_free) is enough: it implies escape analysis.
options gen2
options persistent_heap
options no_aot
options no_optimization
options force_allocate_on_stack = true

require dastest/testing_boost public
require UnitTest

let N = 2000

struct Node {
    x : int
    y : int
}

// over the 256-byte stack-allocation cap (80 * 4 = 320 bytes): stays on the heap and is freed at
// scope exit by the scope-free pass, so the stack frame can't be blown by a big non-escaping object
struct BigNode {
    data : int[80]
}

// owns a heap field: the struct shell is in the frame, but `data`'s backing is still on the heap
// and must be reclaimed by the scope-exit raw collect
struct OwningNode {
    tag : int
    data : array<int>
}

var g_list : array<Node?>

// `new Node()` (no initializer) - ExprNew -> SimNode_NewStack, no heap touch at all
def sum_plain_new() : int {
    var total = 0
    for (i in range(N)) {
        var p = new Node()  // nolint:STYLE013 - deliberately the no-initializer ExprNew lowering
        p.x = i
        p.y = i * 2
        total += p.x + p.y
    }
    return total
}

// `new Node(x = .., y = ..)` - ExprAscend over make-local -> built directly in the frame
def sum_ascend_new() : int {
    var total = 0
    for (i in range(N)) {
        var p = new Node(x = i, y = i * 2)
        total += p.x + p.y
    }
    return total
}

// shell on the stack; the owned array is still on the heap and is freed at scope exit
def sum_owning() : int {
    var total = 0
    for (i in range(N)) {
        var p = new OwningNode(tag = i, data <- [for (x in range(20)); x])
        total += p.tag + length(p.data)
    }
    return total
}

// big non-escaping struct: over the cap, so heap + scope-free (not stack). Verifies the cap path
// still works and keeps the heap bounded.
def sum_big() : int {
    var total = 0
    for (i in range(N)) {
        var p = new BigNode()
        p.data[0] = i
        total += p.data[0]
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

// plain `new` with field stores -> stack frame -> heap stays EXACTLY flat (zero growth)
[test]
def test_stack_plain_new(t : T?) {
    let before = heap_bytes_allocated()
    let s = sum_plain_new()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, 3 * ((N - 1) * N / 2))
    t |> equal(grew, uint64(0))
}

// `new T(f = v)` (ascend) -> built in the frame -> heap stays EXACTLY flat
[test]
def test_stack_ascend_new(t : T?) {
    let before = heap_bytes_allocated()
    let s = sum_ascend_new()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, 3 * ((N - 1) * N / 2))
    t |> equal(grew, uint64(0))
}

// shell in the frame, owned arrays freed at scope exit -> heap stays flat (a few slots at most)
[test]
def test_stack_owning_frees_owned_heap(t : T?) {
    let before = heap_bytes_allocated()
    let s = sum_owning()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, (N - 1) * N / 2 + N * 20)
    t |> success(grew < uint64(40 * (typeinfo sizeof(type<OwningNode>))))
}

// over-the-cap struct: stays on the heap (not stack), still freed at scope exit -> heap bounded
[test]
def test_stack_big_struct_stays_on_heap(t : T?) {
    let before = heap_bytes_allocated()
    let s = sum_big()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, (N - 1) * N / 2)
    t |> success(grew < uint64(40 * (typeinfo sizeof(type<BigNode>))))
}

// control: an escaping pointer must stay on the heap and the heap must grow by N nodes
[test]
def test_stack_control_grows(t : T?) {
    g_list |> clear()
    let before = heap_bytes_allocated()
    let s = sum_escaping()
    let grew = heap_bytes_allocated() - before
    t |> equal(s, 3 * ((N - 1) * N / 2))
    t |> equal(length(g_list), N)
    t |> success(grew >= uint64(N) * (typeinfo sizeof(type<Node>)))
}
