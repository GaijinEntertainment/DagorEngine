options gen2
require UnitTest
require dastest/testing_boost public

[test]
def test_value_table_key(t : T?) {
    var tab : table<EntityId; string>  // nolint:STYLE031 — insert with value keys is under test
    tab |> insert(EntityId(1), "hello")
    tab |> insert(EntityId(2), "world")
    tab |> insert(EntityId(3), "!")
    t |> equal(length(tab), 3)
    tab |> erase(EntityId(3))
    t |> equal(length(tab), 2)

    var found = tab |> get(EntityId(2)) $(val) {
        t |> equal(val, "world")
    }
    t |> success(found)

    found = tab |> get(EntityId(1)) $(val) {
        t |> equal(val, "hello")
    }
    t |> success(found)

    t |> success(tab |> key_exists(EntityId(1)))
    t |> success(!(tab |> key_exists(EntityId(3))))

    let val = tab ?[EntityId(1)] ?? "goodbye"
    t |> equal(val, "hello")
    var ptab = unsafe(addr(tab))
    let q = ptab ?[EntityId(2)] ?? "goodbye"
    t |> equal(q, "world")

    var count = 0
    for (k, v in keys(tab), values(tab)) {
        if (int(k) == 1) {
            t |> equal(v, "hello")
        }
        if (int(k) == 2) {
            t |> equal(v, "world")
        }
        count ++
    }
    t |> equal(count, 2)

    var set : table<EntityId>  // nolint:STYLE031 — set insert with value keys is under test
    set |> insert(EntityId(1))
    set |> insert(EntityId(2))
    set |> insert(EntityId(3))
    t |> equal(length(set), 3)
    t |> success(set |> key_exists(EntityId(3)))
    t |> success(!(set |> key_exists(EntityId(4))))
}

// A value type whose workhorse is vec4f rather than an integer: das compares the workhorse, so
// the C++ type needs no operator == of its own, and a key is the same key in all three tiers.
[test]
def test_wide_value_table_key(t : T?) {
    let a = unsafe(reinterpret<BigEntityId>(int4(1, 2, 3, 4)))
    let b = unsafe(reinterpret<BigEntityId>(int4(9, 8, 7, 6)))
    var tab : table<BigEntityId; string>  // nolint:STYLE031 — insert with value keys is under test
    tab |> insert(a, "alpha")
    tab |> insert(b, "beta")
    t |> equal(length(tab), 2)
    t |> success(tab |> key_exists(a))
    t |> success(tab |> key_exists(b))
    t |> success(!(tab |> key_exists(BigEntityId())))
    let va = tab ?[a] ?? "none"
    t |> equal(va, "alpha")
    let vb = tab ?[b] ?? "none"
    t |> equal(vb, "beta")
    tab |> erase(a)
    t |> equal(length(tab), 1)
    t |> success(!(tab |> key_exists(a)))
}
