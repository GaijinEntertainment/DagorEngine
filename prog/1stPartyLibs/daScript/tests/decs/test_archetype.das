options gen2
options persistent_heap = true
options gc
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require daslib/decs_boost
require dastest/testing_boost public

// === Archetype creation and lookup ===

[test]
def test_single_archetype(t : T?) {
    restart()
    for (i in range(5)) {
        create_entity() @(eid, cmp) {
            cmp.pos := float3(i)
        }
    }
    commit()
    t |> run("one archetype") @(t : T?) {
        t |> equal(length(decsState.allArchetypes), 1)
    }
    t |> run("correct size") @(t : T?) {
        t |> equal(decsState.allArchetypes[0].size, 5)
    }
    t |> run("has eid component") @(t : T?) {
        t |> success(decsState.allArchetypes[0] |> has("eid"))
    }
    t |> run("has pos component") @(t : T?) {
        t |> success(decsState.allArchetypes[0] |> has("pos"))
    }
    t |> run("no vel component") @(t : T?) {
        t |> success(!(decsState.allArchetypes[0] |> has("vel")))
    }
}

[test]
def test_multiple_archetypes(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp.a := 1
    }
    create_entity() @(eid, cmp) {
        cmp.b := 2.0
    }
    create_entity() @(eid, cmp) {
        cmp.a := 3
        cmp.b := 4.0
    }
    commit()
    t |> run("three distinct archetypes") @(t : T?) {
        t |> equal(length(decsState.allArchetypes), 3)
    }
}

[test]
def test_same_components_same_archetype(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp.x := 1
        cmp.y := 2
    }
    create_entity() @(eid, cmp) {
        cmp.x := 10
        cmp.y := 20
    }
    commit()
    t |> run("same archetype") @(t : T?) {
        t |> equal(length(decsState.allArchetypes), 1)
    }
    t |> run("two entities in it") @(t : T?) {
        t |> equal(decsState.allArchetypes[0].size, 2)
    }
}

[test]
def test_archetype_lookup(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp.hp := 100
    }
    create_entity() @(eid, cmp) {
        cmp.hp := 50
        cmp.armor := 10
    }
    commit()
    t |> run("lookup table has entries") @(t : T?) {
        var count = 0
        for (k, v in keys(decsState.archetypeLookup), values(decsState.archetypeLookup)) {
            count++
        }
        t |> equal(count, 2)
    }
}

// === Entity removal shrinks archetype ===

[test]
def test_archetype_shrinks_on_delete(t : T?) {
    restart()
    var eids : array<EntityId>
    for (i in range(4)) {
        let eid = create_entity() @(eid, cmp) {
            cmp.val := i
        }
        eids |> push(eid)
    }
    commit()
    t |> equal(decsState.allArchetypes[0].size, 4)
    delete_entity(eids[1])
    delete_entity(eids[2])
    commit()
    t |> equal(decsState.allArchetypes[0].size, 2)
    // remaining entities still have correct data
    var vals : array<int>
    query() $(val : int) {
        vals |> push(val)
    }
    sort(vals)
    t |> equal(length(vals), 2)
    t |> equal(vals[0], 0)
    t |> equal(vals[1], 3)
}

// === Archetype changes on update ===

[test]
def test_update_entity_changes_archetype(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.x := 1
    }
    commit()
    t |> run("initially 1 archetype") @(t : T?) {
        t |> equal(length(decsState.allArchetypes), 1)
    }
    update_entity(eid) @(eid, cmp) {
        cmp.y := 2
    }
    commit()
    t |> run("now 2 archetypes") @(t : T?) {
        t |> equal(length(decsState.allArchetypes), 2)
    }
    t |> run("old archetype is empty") @(t : T?) {
        // one of them has size 0
        var emptyCount = 0
        for (arch in decsState.allArchetypes) {
            if (arch.size == 0) {
                emptyCount++
            }
        }
        t |> equal(emptyCount, 1)
    }
    // entity is queryable in new archetype
    var found = false
    query(eid) $(x : int; y : int) {
        t |> equal(x, 1)
        t |> equal(y, 2)
        found = true
    }
    t |> success(found)
}

// === Empty world queries ===

[test]
def test_query_empty_world(t : T?) {
    restart()
    var count = 0
    query() $(pos : float3) {
        count++
    }
    t |> equal(count, 0)
}

[test]
def test_find_query_empty_world(t : T?) {
    restart()
    let found = find_query() $(pos : float3) {
        return true
    }
    t |> equal(found, false)
}

// === Entity lookup table ===

[test]
def test_entity_lookup(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.tag := 42
    }
    commit()
    t |> run("lookup table sized correctly") @(t : T?) {
        t |> success(int(eid.id) < length(decsState.entityLookup))
    }
    t |> run("generation matches") @(t : T?) {
        t |> equal(decsState.entityLookup[eid.id].generation, eid.generation)
    }
}
