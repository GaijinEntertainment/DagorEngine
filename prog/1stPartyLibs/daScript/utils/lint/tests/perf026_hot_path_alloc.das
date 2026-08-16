options gen2
options auto_inline_functions = false   // lint fixtures assert SOURCE shapes; splices rewrite them
// PERF026: heap traffic on a hot path
//
// Problem:
//   Allocating or freeing inside a per-frame / per-token loop is the classic
//   hidden cost — an array resize in the middle of a decode step, a table
//   insert per layer. The cost is invisible at the call site because the
//   allocation happens several calls deep, inside daslib.
//
// Bad pattern:
//   [hot_path]
//   def step(var s : Session) {
//       s.stages |> resize(4)
//   }
//
// Good pattern:
//   size the buffer once at setup; declare a reused buffer @scratch at its
//   field (or at the parameter of a helper that sizes one by reference); or
//   put a genuinely one-time growth behind a [cold_path] helper.
//
// The rule is annotation-gated: nothing is checked until a function carries
// [hot_path] (all three contracts) or the narrow [no_alloc] / [no_env] /
// [no_io]. From each root the scan follows DIRECT calls transitively, so a
// sink several frames deep is still reported — anchored on the deepest line
// written in the root's own file, not on the daslib internal that allocates.

expect 31208:7

require daslib/perf_lint

struct Blob {
    v : int
}

// A reused buffer says so once, at the declaration. Sizing it to the current step's geometry is
// the owner's allocation strategy, not an accident at each call site — and unlike a nolint per
// line, the statement survives the code moving.
struct Scratch {
    @scratch reused : array<float>
    fresh : array<float>
}

// 1. allocation written directly in the annotated function
[no_alloc]
def bad_direct(var a : array<int>) {
    a |> push(1)
}

// 2. allocation reached through an ordinary (unannotated) helper — the helper is
//    scanned because it is reachable, and the warning lands on the push line
def alloc_helper(var a : array<int>) {
    a |> reserve(64)
}

[no_alloc]
def bad_transitive(var a : array<int>) {
    alloc_helper(a)
}

// 3. [cold_path] prunes the walk: one-time setup reached from a hot function is fine
[cold_path]
def cold_init(var a : array<int>) {
    a |> reserve(1024)
}

[no_alloc]
def good_cold(var a : array<int>) {
    cold_init(a)
}

// 4. per-line suppression still works
[no_alloc]
def good_nolint(var a : array<int>) {
    a |> push(2)   // nolint:PERF026
}

// 5. an unannotated function is never scanned
def good_unannotated(var a : array<int>) {
    a |> push(3)
}

// 6. a @scratch destination is declared intent and stays silent; its unmarked neighbour does not
[no_alloc]
def bad_only_the_unmarked_buffer(var sc : Scratch; n : int) {
    sc.reused |> resize(n)
    sc.fresh |> resize(n)
}

// 7. a helper that sizes a caller's buffer BY REFERENCE marks the parameter — the call site
//    cannot see which field arrived
def ensure_room(@scratch var a : array<float>; n : int) {
    a |> resize(n)
}

[no_alloc]
def good_scratch_helper(var sc : Scratch; n : int) {
    ensure_room(sc.reused, n)
}

// 8. new and delete are heap traffic too
[no_alloc]
def bad_new_delete() : int {
    var p = new Blob(v = 7)
    let v = p.v
    unsafe {
        delete p
    }
    return v
}

// 9. a @scratch module GLOBAL is declared intent too — the capture-rail shape: a
//    clear()-recycled array that grows once and is reused; its unmarked neighbour still reports
var @scratch g_reused : array<int>
var g_fresh : array<int>

[no_alloc]
def bad_only_the_unmarked_global() {
    g_reused |> push(1)
    g_reused |> pop()   // pop rides the escape too (shrink of a reused buffer is a store)
    g_fresh |> push(2)
}

// 10. indexing a @scratch TABLE is the declared pool / residency-cache shape (insert-on-first-
//     touch, steady-state lookup); an unmarked table's [] still reports its insert-on-read
var @scratch g_pool : table<int; int>
var g_plain : table<int; int>

[no_alloc]
def bad_only_the_unmarked_table(k : int) : int {
    unsafe {
        return g_pool[k] + g_plain[k]
    }
}

// 11. a local REFERENCE binding carries its destination's scratch-ness (the pool free-list
//     idiom: `var lst & = pool.buckets[b]`)
var @scratch g_buckets : table<int; array<int>>

[no_alloc]
def good_scratch_through_ref(b : int) : int {
    var lst & = unsafe(g_buckets[b])
    lst |> push(1)
    let v = lst[length(lst) - 1]
    lst |> pop()
    return v
}

// 12. a nolint ANYWHERE along the chain suppresses — the sink may bottom in daslib, so the
//     honest line is often an intermediate call site in a required module
def deep_alloc12(var a : array<int>) {
    a |> reserve(64)
}

def mid_hop12(var a : array<int>) {
    deep_alloc12(a)   // nolint:PERF026 — chain-middle suppression is honored
}

[no_alloc]
def good_chain_middle_nolint(var a : array<int>) {
    mid_hop12(a)
}
