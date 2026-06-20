// to_table_move must MOVE (consume + free) its source array, not copy it.
//
// A table/set comprehension lowers to "build a temp array<KV> in a closure,
// then return to_table_move(temp)". The temp is a closure local that is never
// returned and never deleted, so if to_table_move only COPIES from it the temp
// buffer is orphaned and leaks ~the table's storage on every evaluation. The
// fix makes to_table_move consume its source (move values out, then delete the
// array). We assert that contract directly — length(source) == 0 afterwards —
// which is deterministic in every execution mode (the equivalent heap-delta
// check is unreliable under the dastest JIT harness, which reports the same
// per-comprehension residue with and without the fix).
//
// The move path also fixes a use-after-free: copying a pointer value (e.g.
// JsonValue?) would alias the node into the table, and the source-free would
// then delete the live node. Moving leaves the element empty, so the heap
// key/value cases below also guard that crash (delete of the result must not
// double-free).
options gen2
options no_unused_function_arguments = false

require dastest/testing_boost public

struct PtrKeyNode {
    x : int
}

// --- set overload: to_table_move(var a : array<keyT>) ---

[test]
def test_to_table_move_set_consumes_source(t : T?) {
    var ks <- [10, 20, 30]
    var tab <- to_table_move(ks)
    t |> equal(length(ks), 0)                       // source moved-from, not copied
    t |> equal(length(tab), 3)
    t |> success(key_exists(tab, 20), "moved key present")
    delete tab
}

[test]
def test_to_table_move_set_heap_keys(t : T?) {
    var ks <- ["a", "b"]
    var tab <- to_table_move(ks)
    t |> equal(length(ks), 0)
    t |> success(key_exists(tab, "a") && key_exists(tab, "b"), "moved heap keys present")
    delete tab
}

// --- map overload: to_table_move(var a : array<tuple<keyT;valT>>) ---

[test]
def test_to_table_move_map_consumes_source(t : T?) {
    var kvs <- [("a", 1), ("b", 2)]
    var tab <- to_table_move(kvs)
    t |> equal(length(kvs), 0)                      // source moved-from, not copied
    t |> equal(tab?["a"] ?? -1, 1)
    t |> equal(tab?["b"] ?? -1, 2)
    delete tab
}

// heap key AND heap value: exercises the move-then-delete path that would
// double-free if the value were pointer-copied (aliased) into the table.
[test]
def test_to_table_move_map_heap_key_value(t : T?) {
    var kvs <- [("k1", "v1"), ("k2", "v2")]
    var tab <- to_table_move(kvs)
    t |> equal(length(kvs), 0)
    t |> equal(tab?["k1"] ?? "?", "v1")
    t |> equal(tab?["k2"] ?? "?", "v2")
    delete tab
}

// --- pointer keys must NOT be freed by the source-reclaim ---

// A table copies its key as a bare pointer value (it does not clone the
// pointee). Reclaiming the temp array must therefore leave pointer-key pointees
// alive, or the table's keys dangle and the caller's data is corrupted. The
// caller retains ownership of the pointees.
[test]
def test_to_table_move_pointer_keys_not_dangling(t : T?) {
    var n0 = new PtrKeyNode(x = 11)
    var n1 = new PtrKeyNode(x = 22)

    // set overload (array<keyT>)
    var ks <- [n0, n1]
    var s <- to_table_move(ks)
    var ssum = 0
    for (k in keys(s)) {
        ssum += k.x
    }
    t |> equal(ssum, 33)                // keys still reference live nodes
    delete s

    // map overload (array<tuple<keyT;valT>>), pointer key + int value
    var kvs <- [(n0, 100), (n1, 200)]
    var m <- to_table_move(kvs)
    var ksum = 0
    for (k in keys(m)) {
        ksum += k.x
    }
    t |> equal(ksum, 33)
    t |> equal(m?[n0] ?? -1, 100)
    delete m

    unsafe {
        delete n0
        delete n1
    }                                   // caller owns the pointees
}

// --- end-to-end: the comprehension forms that lower to the overloads above ---

[test]
def test_comprehension_round_trip(t : T?) {
    var setc <- {for (j in range(4)); "k{j}"}
    t |> equal(length(setc), 4)
    t |> success(key_exists(setc, "k2"), "set comprehension element present")
    delete setc

    var mapc <- {for (j in range(4)); "k{j}" => j * j}
    t |> equal(length(mapc), 4)
    t |> equal(mapc?["k3"] ?? -1, 9)
    delete mapc
}
