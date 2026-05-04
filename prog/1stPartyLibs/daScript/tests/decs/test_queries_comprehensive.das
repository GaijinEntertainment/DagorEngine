options gen2
options persistent_heap = true
options gc
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require daslib/decs_boost
require dastest/testing_boost public

// === Query across multiple archetypes ===

[test]
def test_query_spans_archetypes(t : T?) {
    restart()
    // {hp} archetype
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp.hp := 100 + i
        }
    }
    // {hp, armor} archetype
    for (i in range(2)) {
        create_entity() @(eid, cmp) {
            cmp.hp := 200 + i
            cmp.armor := 50
        }
    }
    commit()
    // query for hp should span both archetypes
    var total = 0
    var hpSum = 0
    query() $(hp : int) {
        total++
        hpSum += hp
    }
    t |> equal(total, 5)
    t |> equal(hpSum, 100 + 101 + 102 + 200 + 201)
}

// === Mutable query ===

[test]
def test_mutable_query(t : T?) {
    restart()
    for (i in range(5)) {
        create_entity() @(eid, cmp) {
            cmp.val := i
        }
    }
    commit()
    // mutate all values
    query() $(var val : int&) {
        val *= 10
    }
    // verify
    var sum = 0
    query() $(val : int) {
        sum += val
    }
    t |> equal(sum, 0 + 10 + 20 + 30 + 40)
}

// === REQUIRE / REQUIRE_NOT ===

[test]
def test_require_filters(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp.name := "soldier"
        cmp.hp := 100
        cmp.weapon := "rifle"
    }
    create_entity() @(eid, cmp) {
        cmp.name := "medic"
        cmp.hp := 80
    }
    create_entity() @(eid, cmp) {
        cmp.name := "tank"
        cmp.hp := 500
        cmp.weapon := "cannon"
        cmp.heavy := true
    }
    commit()

    // REQUIRE weapon — should get soldier + tank
    var withWeapon = 0
    query <| $ [REQUIRE(weapon)] (hp : int) {
        withWeapon++
    }
    t |> equal(withWeapon, 2)

    // REQUIRE_NOT weapon — should get medic only
    var withoutWeapon = 0
    query <| $ [REQUIRE_NOT(weapon)] (hp : int) {
        withoutWeapon++
    }
    t |> equal(withoutWeapon, 1)

    // REQUIRE heavy — should get only tank
    var heavyCount = 0
    query <| $ [REQUIRE(heavy)] (hp : int) {
        heavyCount++
    }
    t |> equal(heavyCount, 1)

    // REQUIRE weapon, REQUIRE_NOT heavy — should get soldier only
    var lightArmed = 0
    query <| $ [REQUIRE(weapon), REQUIRE_NOT(heavy)] (hp : int) {
        lightArmed++
    }
    t |> equal(lightArmed, 1)
}

// === find_query returns true on match, false on no match ===

[test]
def test_find_query_result(t : T?) {
    restart()
    for (i in range(10)) {
        create_entity() @(eid, cmp) {
            cmp.idx := i
        }
    }
    commit()
    let found = find_query() $(idx : int) {
        if (idx == 5) {
            return true
        }
        return false
    }
    t |> success(found)

    let notFound = find_query() $(idx : int) {
        if (idx == 99) {
            return true
        }
        return false
    }
    t |> success(!notFound)
}

// === Query by EntityId ===

[test]
def test_query_by_eid_returns_correct_entity(t : T?) {
    restart()
    var eids : array<EntityId>
    for (i in range(5)) {
        let eid = create_entity() @(eid, cmp) {
            cmp.idx := i
        }
        eids |> push(eid)
    }
    commit()
    // query specific eid should return its value
    for (i in range(5)) {
        var foundIdx = -1
        query(eids[i]) $(idx : int) {
            foundIdx = idx
        }
        t |> equal(foundIdx, i)
    }
}

// === Query with default values ===

[test]
def test_query_default_value(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp.x := 10
        cmp.y := 20
    }
    create_entity() @(eid, cmp) {
        cmp.x := 30
    }
    commit()
    var results : array<int2>
    query() $(x : int; y : int = -1) {
        results |> push(int2(x, y))
    }
    sort(results) <| $(a, b : int2) => a.x < b.x
    t |> equal(length(results), 2)
    t |> equal(results[0], int2(10, 20))   // has y component
    t |> equal(results[1], int2(30, -1))   // gets default
}

// === Large entity count ===

[test]
def test_large_entity_count(t : T?) {
    restart()
    for (i in range(1000)) {
        create_entity() @(eid, cmp) {
            cmp.idx := i
        }
    }
    commit()
    var count = 0
    var sum = 0
    query() $(idx : int) {
        count++
        sum += idx
    }
    t |> equal(count, 1000)
    t |> equal(sum, 999 * 1000 / 2)   // sum 0..999
}

// === Nested queries ===

[test]
def test_nested_query(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp.a := 1
        cmp.b := 10
    }
    create_entity() @(eid, cmp) {
        cmp.a := 2
        cmp.b := 20
    }
    commit()
    var pairSum = 0
    query() $(a : int) {
        query() $(b : int) {
            pairSum += a * b
        }
    }
    // Every a sees every b: (1*10 + 1*20) + (2*10 + 2*20) = 30 + 60 = 90
    t |> equal(pairSum, 90)
}

// === Query after delete ===

[test]
def test_query_after_delete(t : T?) {
    restart()
    let e1 = create_entity() @(eid, cmp) {
        cmp.val := 1
    }
    let e2 = create_entity() @(eid, cmp) {
        cmp.val := 2
    }
    let e3 = create_entity() @(eid, cmp) {
        cmp.val := 3
    }
    commit()
    delete_entity(e2)
    commit()
    var vals : array<int>
    query() $(val : int) {
        vals |> push(val)
    }
    sort(vals)
    t |> equal(length(vals), 2)
    t |> equal(vals[0], 1)
    t |> equal(vals[1], 3)
}
