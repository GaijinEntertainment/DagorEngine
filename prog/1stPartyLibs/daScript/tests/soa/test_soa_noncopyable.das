options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/soa

// ===========================
// Struct with a non-copyable field (array)
// ===========================

[soa]
struct Group {
    name   : string
    scores : array<int>
    active : bool
}

// Helper: build a Group with some scores
def make_group(n : string; vals : array<int>; act : bool) : Group {
    var g = Group(name = n, active = act)
    for (v in vals) {
        g.scores |> push(v)
    }
    return <- g
}

// ===========================
// Push must not destroy the source variable
// ===========================

[test]
def test_noncopyable_push_preserves_source(t : T?) {
    t |> run("push named variable preserves source scores") @(t : T?) {
        var soa : Group`SOA
        var g <- make_group("alpha", [1, 2, 3], true)
        soa |> push(g)
        // g must still have its data — push should clone, not move
        t |> equal(g.name, "alpha")
        t |> equal(length(g.scores), 3)
        t |> equal(g.scores[0], 1)
        t |> equal(g.scores[1], 2)
        t |> equal(g.scores[2], 3)
        t |> equal(g.active, true)
        // SOA should also have the data
        t |> equal(length(soa), 1)
        t |> equal(soa[0].name, "alpha")
        t |> equal(length(soa[0].scores), 3)
    }
    t |> run("push named variable twice works") @(t : T?) {
        var soa : Group`SOA
        var g <- make_group("x", [10, 20], false)
        soa |> push(g)
        soa |> push(g)
        t |> equal(length(soa), 2)
        // Both copies should have the data
        t |> equal(soa[0].name, "x")
        t |> equal(soa[1].name, "x")
        t |> equal(length(soa[0].scores), 2)
        t |> equal(length(soa[1].scores), 2)
        // Source should be intact
        t |> equal(g.name, "x")
        t |> equal(length(g.scores), 2)
    }
}

// ===========================
// Basic: push with move, length, indexed access
// ===========================

[test]
def test_noncopyable_push_and_access(t : T?) {
    t |> run("push with move and length") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("alpha", [1, 2, 3], true)
        soa |> push <| make_group("beta", [10, 20], false)
        t |> equal(length(soa), 2)
    }
    t |> run("indexed access on name") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("alpha", [1, 2, 3], true)
        soa |> push <| make_group("beta", [10, 20], false)
        t |> equal(soa[0].name, "alpha")
        t |> equal(soa[1].name, "beta")
    }
    t |> run("indexed access on array field") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("alpha", [1, 2, 3], true)
        soa |> push <| make_group("beta", [10, 20], false)
        t |> equal(length(soa[0].scores), 3)
        t |> equal(soa[0].scores[0], 1)
        t |> equal(soa[0].scores[2], 3)
        t |> equal(length(soa[1].scores), 2)
        t |> equal(soa[1].scores[0], 10)
    }
    t |> run("indexed access on bool field") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("alpha", [1, 2, 3], true)
        soa |> push <| make_group("beta", [10, 20], false)
        t |> equal(soa[0].active, true)
        t |> equal(soa[1].active, false)
    }
}

// ===========================
// Iteration
// ===========================

[test]
def test_noncopyable_iteration(t : T?) {
    t |> run("for loop accesses array field") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("a", [1, 2], true)
        soa |> push <| make_group("b", [3, 4, 5], false)
        var total = 0
        for (it in soa) {
            for (s in it.scores) {
                total += s
            }
        }
        t |> equal(total, 15)
    }
    t |> run("for loop accesses mixed fields") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("x", [100], true)
        soa |> push <| make_group("y", [200, 300], false)
        var names = ""
        var score_count = 0
        for (it in soa) {
            names += it.name + ";"
            score_count += length(it.scores)
        }
        t |> equal(names, "x;y;")
        t |> equal(score_count, 3)
    }
}

// ===========================
// Erase
// ===========================

[test]
def test_noncopyable_erase(t : T?) {
    t |> run("erase removes element with array field") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("a", [1, 2], true)
        soa |> push <| make_group("b", [3], false)
        soa |> push <| make_group("c", [4, 5, 6], true)
        soa |> erase(1)
        t |> equal(length(soa), 2)
        t |> equal(soa[0].name, "a")
        t |> equal(soa[1].name, "c")
        t |> equal(length(soa[1].scores), 3)
        t |> equal(soa[1].scores[0], 4)
    }
}

// ===========================
// Pop
// ===========================

[test]
def test_noncopyable_pop(t : T?) {
    t |> run("pop removes last element with array field") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("a", [1, 2], true)
        soa |> push <| make_group("b", [3, 4], false)
        soa |> pop()
        t |> equal(length(soa), 1)
        t |> equal(soa[0].name, "a")
        t |> equal(length(soa[0].scores), 2)
    }
}

// ===========================
// Clear
// ===========================

[test]
def test_noncopyable_clear(t : T?) {
    t |> run("clear removes all elements with array fields") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("a", [1], true)
        soa |> push <| make_group("b", [2], false)
        soa |> clear()
        t |> equal(length(soa), 0)
    }
}

// ===========================
// Resize and reserve
// ===========================

[test]
def test_noncopyable_resize(t : T?) {
    t |> run("resize truncates array fields") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("a", [1, 2], true)
        soa |> push <| make_group("b", [3], false)
        soa |> push <| make_group("c", [4, 5], true)
        soa |> resize(2)
        t |> equal(length(soa), 2)
        t |> equal(soa[1].name, "b")
    }
    t |> run("resize extends with defaults") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("a", [1], true)
        soa |> resize(3)
        t |> equal(length(soa), 3)
        // extended elements have default array (empty)
        t |> equal(length(soa[1].scores), 0)
        t |> equal(length(soa[2].scores), 0)
        t |> equal(soa[1].name, "")
        t |> equal(soa[2].active, false)
    }
}

[test]
def test_noncopyable_reserve(t : T?) {
    t |> run("reserve does not change length") @(t : T?) {
        var soa : Group`SOA
        soa |> reserve(50)
        t |> equal(length(soa), 0)
        soa |> push <| make_group("a", [1], true)
        t |> equal(length(soa), 1)
        t |> success(capacity(soa) >= 50, "capacity should be >= 50")
    }
}

// ===========================
// Capacity
// ===========================

[test]
def test_noncopyable_capacity(t : T?) {
    t |> run("capacity on struct with array field") @(t : T?) {
        var soa : Group`SOA
        soa |> reserve(100)
        t |> success(capacity(soa) >= 100, "capacity should be >= 100")
    }
}

// ===========================
// Swap
// ===========================

[test]
def test_noncopyable_swap(t : T?) {
    t |> run("swap exchanges elements with array fields") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("first", [1, 2], true)
        soa |> push <| make_group("second", [3, 4, 5], false)
        soa |> swap(0, 1)
        t |> equal(soa[0].name, "second")
        t |> equal(soa[1].name, "first")
        t |> equal(length(soa[0].scores), 3)
        t |> equal(soa[0].scores[0], 3)
        t |> equal(length(soa[1].scores), 2)
        t |> equal(soa[1].scores[0], 1)
        t |> equal(soa[0].active, false)
        t |> equal(soa[1].active, true)
    }
    t |> run("swap same index is no-op") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("only", [99], true)
        soa |> swap(0, 0)
        t |> equal(soa[0].name, "only")
        t |> equal(soa[0].scores[0], 99)
    }
}

// ===========================
// push_clone with non-copyable field
// ===========================

[test]
def test_noncopyable_push_clone(t : T?) {
    t |> run("push_clone with non-copyable field") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("a", [1, 2, 3], true)
        // Construct a separate Group value to push_clone
        var g <- make_group("b", [4, 5], false)
        soa |> push_clone(g)
        t |> equal(length(soa), 2)
        t |> equal(soa[0].name, "a")
        t |> equal(soa[1].name, "b")
        t |> equal(length(soa[1].scores), 2)
        t |> equal(soa[1].scores[0], 4)
        t |> equal(soa[1].scores[1], 5)
        t |> equal(soa[1].active, false)
    }
    t |> run("push_clone does not consume source") @(t : T?) {
        var soa : Group`SOA
        var g <- make_group("src", [10, 20, 30], true)
        soa |> push_clone(g)
        soa |> push_clone(g)
        t |> equal(length(soa), 2)
        // Both copies should have the same data
        t |> equal(soa[0].name, "src")
        t |> equal(soa[1].name, "src")
        t |> equal(length(soa[0].scores), 3)
        t |> equal(length(soa[1].scores), 3)
        // Source should be unchanged
        t |> equal(g.name, "src")
        t |> equal(length(g.scores), 3)
    }
}

// ===========================
// Modify array field through indexed access
// ===========================

[test]
def test_noncopyable_modify_array_field(t : T?) {
    t |> run("modify array field via indexed access") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("a", [1, 2], true)
        soa[0].scores |> push(3)
        t |> equal(length(soa[0].scores), 3)
        t |> equal(soa[0].scores[2], 3)
    }
    t |> run("clear array field via indexed access") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("a", [10, 20, 30], true)
        soa[0].scores |> clear()
        t |> equal(length(soa[0].scores), 0)
    }
    t |> run("modify array field in for loop") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("a", [1, 2], true)
        soa |> push <| make_group("b", [3, 4], false)
        for (it in soa) {
            it.scores |> push(99)
        }
        t |> equal(length(soa[0].scores), 3)
        t |> equal(soa[0].scores[2], 99)
        t |> equal(length(soa[1].scores), 3)
        t |> equal(soa[1].scores[2], 99)
    }
}

// ===========================
// from_array with non-copyable field
// ===========================

[test]
def test_noncopyable_from_array(t : T?) {
    t |> run("from_array clones non-copyable fields") @(t : T?) {
        var arr : array<Group>
        arr |> emplace <| make_group("a", [1, 2], true)
        arr |> emplace <| make_group("b", [3, 4, 5], false)
        var soa : Group`SOA
        soa |> from_array(arr)
        t |> equal(length(soa), 2)
        t |> equal(soa[0].name, "a")
        t |> equal(soa[1].name, "b")
        t |> equal(length(soa[0].scores), 2)
        t |> equal(soa[0].scores[0], 1)
        t |> equal(length(soa[1].scores), 3)
        t |> equal(soa[1].scores[1], 4)
        t |> equal(soa[0].active, true)
        t |> equal(soa[1].active, false)
    }
    t |> run("from_array preserves source array") @(t : T?) {
        var arr : array<Group>
        arr |> emplace <| make_group("x", [10], true)
        var soa : Group`SOA
        soa |> from_array(arr)
        // Source array should be unchanged
        t |> equal(length(arr), 1)
        t |> equal(arr[0].name, "x")
        t |> equal(length(arr[0].scores), 1)
        t |> equal(arr[0].scores[0], 10)
    }
}

// ===========================
// to_array with non-copyable field
// ===========================

[test]
def test_noncopyable_to_array(t : T?) {
    t |> run("to_array clones non-copyable fields") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("a", [1, 2], true)
        soa |> push <| make_group("b", [3, 4, 5], false)
        var result <- to_array(soa)
        t |> equal(length(result), 2)
        t |> equal(result[0].name, "a")
        t |> equal(result[1].name, "b")
        t |> equal(length(result[0].scores), 2)
        t |> equal(result[0].scores[0], 1)
        t |> equal(length(result[1].scores), 3)
        t |> equal(result[1].scores[1], 4)
        t |> equal(result[0].active, true)
        t |> equal(result[1].active, false)
    }
    t |> run("to_array roundtrip preserves data") @(t : T?) {
        var soa : Group`SOA
        soa |> push <| make_group("p", [7, 8, 9], true)
        soa |> push <| make_group("q", [10], false)
        // SOA → AOS → SOA roundtrip
        var arr <- to_array(soa)
        var soa2 : Group`SOA
        soa2 |> from_array(arr)
        t |> equal(length(soa2), 2)
        t |> equal(soa2[0].name, "p")
        t |> equal(length(soa2[0].scores), 3)
        t |> equal(soa2[0].scores[2], 9)
        t |> equal(soa2[1].name, "q")
        t |> equal(length(soa2[1].scores), 1)
    }
}