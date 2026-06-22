// Coverage for runtime heap GC walk paths not exercised by tests/decs/test_gc.das:
// garbage reclamation, cyclic references (un/reachable), table keys+values, big (bigStuff)
// allocations, and repeated-collect stability. All collects use validate mode so a dangling
// or double-freed pointer fails loudly.
options gen2
options persistent_heap
options gc
options no_aot

require dastest/testing_boost public

struct CNode {
    next : CNode?
}

struct Holder {
    name : string
    vals : array<int>
    tags : table<string; int>
}

// globals are unconditional GC roots
var g_holder : Holder?
var g_cycle : CNode?
var g_big : array<int>
// storing each allocation through a global makes it escape (heap, not stack-allocated by escape
// analysis) so this suite keeps exercising the heap GC; cleared to null so it all becomes garbage
var g_garbage_sink : Holder?

def collect() {
    unsafe {
        heap_collect(true, true)
    }
}

def make_garbage(n : int) {
    for (i in range(n)) {
        g_garbage_sink = new Holder(name = "garbage_{i}", vals <- [for (x in range(8)); x])
    }
    g_garbage_sink = null
}

// reclaim garbage; reachable data survives intact
[test]
def test_gc_frees_garbage(t : T?) {
    g_holder = new Holder()
    g_holder.name := "live"
    g_holder.vals <- [for (x in range(16)); x * 2]
    collect()
    let live = heap_bytes_allocated()
    make_garbage(2000)
    let withGarbage = heap_bytes_allocated()
    t |> success(withGarbage > live, "garbage should grow the heap")
    collect()
    t |> success(heap_bytes_allocated() < withGarbage, "collect must reclaim garbage")
    t |> equal(g_holder.name, "live")
    t |> equal(g_holder.vals[8], 16)
}

// an unreachable cycle must be reclaimed (a refcount collector would leak it)
def make_unreachable_cycle() {
    var a = new CNode()
    var b = new CNode()
    a.next = b
    b.next = a              // a<->b; both go out of scope here -> unreachable cycle
}

[test]
def test_gc_unreachable_cycle_freed(t : T?) {
    g_cycle = null
    collect()
    let before = heap_bytes_allocated()
    make_unreachable_cycle()
    collect()
    t |> success(heap_bytes_allocated() <= before, "unreachable cycle must be reclaimed")
}

// a reachable cycle must survive and must not hang the marker
[test]
def test_gc_reachable_cycle_survives(t : T?) {
    var a = new CNode()
    var b = new CNode()
    a.next = b
    b.next = a
    g_cycle = a
    make_garbage(500)
    collect()
    t |> success(g_cycle != null, "cycle head survived")
    t |> success(g_cycle.next != null && g_cycle.next.next == g_cycle, "cycle preserved through GC")
    g_cycle = null
}

// table with heap keys (strings) and a struct holding it
[test]
def test_gc_table_keys_values(t : T?) {
    g_holder = new Holder()
    g_holder.name := "tbl"
    for (i in range(64)) {
        g_holder.tags |> insert("key_{i}", i * i)
    }
    make_garbage(1000)
    collect()
    t |> equal(length(g_holder.tags), 64)
    t |> equal(g_holder.tags?["key_7"] ?? -1, 49)
}

// large array routes through bigStuff (>256B); must survive a GC alongside churn
[test]
def test_gc_big_allocation(t : T?) {
    g_big <- [for (x in range(2000)); x]
    make_garbage(1000)
    collect()
    t |> equal(length(g_big), 2000)
    t |> equal(g_big[1234], 1234)
}

// repeated collects on a stable live set must not change the heap (no leak, no premature free)
[test]
def test_gc_repeated_stable(t : T?) {
    g_holder = new Holder()
    g_holder.name := "stable"
    g_holder.vals <- [for (x in range(64)); x]
    for (i in range(64)) {
        g_holder.tags |> insert("k_{i}", i)
    }
    collect()
    let b1 = heap_bytes_allocated()
    for (_i in range(5)) {
        collect()
    }
    t |> equal(b1, heap_bytes_allocated())
    t |> equal(g_holder.name, "stable")
    t |> equal(length(g_holder.vals), 64)
}
