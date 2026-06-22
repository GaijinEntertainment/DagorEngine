// Regression test for the bounded-recursion mark walk: a deeply nested heap graph must be
// collected without overflowing the C++ stack. Before the guard, this SIGSEGV'd in the
// recursive GcMarkAnyHeap::walk; now it collects and the live chain survives.
options gen2
options persistent_heap
options gc
options no_aot

require dastest/testing_boost public

struct Node {
    name : string
    next : Node?
}

var g_head : Node?

def build_chain(depth : int) {
    var head : Node? = null
    for (_i in range(depth)) {
        head = new Node(name = "n", next = head)
    }
    g_head = head
}

def chain_len(head : Node?) : int {
    var n = 0
    var p = head
    while (p != null) {
        n ++
        p = p.next
    }
    return n
}

[test]
def test_gc_deep_chain_no_overflow(t : T?) {
    let depth = 200000
    build_chain(depth)
    unsafe {
        heap_collect(true, true)    // validate=true: no overflow, no dangling pointers
    }
    t |> equal(chain_len(g_head), depth)    // whole chain survived GC (reachable, not swept)
}
