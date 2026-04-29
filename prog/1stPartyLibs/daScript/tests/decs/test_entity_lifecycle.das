options gen2
options persistent_heap = true
options gc
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require daslib/decs_boost
require dastest/testing_boost public

[test]
def test_entity_id_uniqueness(t : T?) {
    restart()
    var ids : array<EntityId>
    for (i in range(10)) {
        let eid = create_entity() @(eid, cmp) {
            cmp.value := i
        }
        ids |> push(eid)
    }
    commit()
    // all ids unique
    for (i in range(length(ids))) {
        for (j in range(i + 1, length(ids))) {
            t |> success(ids[i] != ids[j])
        }
    }
    // all ids valid
    for (eid in ids) {
        t |> success(eid != INVALID_ENTITY_ID)
    }
}

[test]
def test_invalid_entity_id(t : T?) {
    restart()
    t |> run("default is invalid") @(t : T?) {
        var eid : EntityId
        t |> equal(eid, INVALID_ENTITY_ID)
    }
    t |> run("invalid entity id equality") @(t : T?) {
        t |> equal(INVALID_ENTITY_ID, INVALID_ENTITY_ID)
    }
}

[test]
def test_entity_id_recycling(t : T?) {
    restart()
    let eid1 = create_entity() @(eid, cmp) {
        cmp.tag := 1
    }
    commit()
    let gen1 = eid1.generation
    let id1 = eid1.id
    // delete and commit
    delete_entity(eid1)
    commit()
    // create again — should reuse the id slot with incremented generation
    let eid2 = create_entity() @(eid, cmp) {
        cmp.tag := 2
    }
    commit()
    t |> run("reuses id slot") @(t : T?) {
        t |> equal(eid2.id, id1)
    }
    t |> run("generation increments") @(t : T?) {
        t |> success(eid2.generation > gen1)
    }
    t |> run("old eid no longer valid") @(t : T?) {
        t |> success(eid1 != eid2)
    }
    t |> run("new entity has correct data") @(t : T?) {
        var found = false
        query(eid2) $(tag : int) {
            t |> equal(tag, 2)
            found = true
        }
        t |> success(found)
    }
    t |> run("old eid query is empty") @(t : T?) {
        var found = false
        query(eid1) $(tag : int) {
            found = true
        }
        t |> success(!found)
    }
}

[test]
def test_multiple_recycle_cycles(t : T?) {
    restart()
    var lastId : uint
    for (cycle in range(5)) {
        let eid = create_entity() @(eid, cmp) {
            cmp.cycle := cycle
        }
        commit()
        if (cycle > 0) {
            t |> equal(eid.id, lastId)
        }
        lastId = eid.id
        delete_entity(eid)
        commit()
    }
    t |> success(true)
}

[test]
def test_delete_already_deleted(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.x := 1
    }
    commit()
    delete_entity(eid)
    commit()
    // delete again — should be a no-op (generation mismatch)
    delete_entity(eid)
    commit()
    t |> success(true)
}

[test]
def test_entity_minimal(t : T?) {
    // entity with a single component — the simplest useful entity
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.tag := 1
    }
    commit()
    var found = false
    query(eid) $(tag : int) {
        t |> equal(tag, 1)
        found = true
    }
    t |> success(found)
}

[test]
def test_entity_many_component_types(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.i  := 42
        cmp.f  := 3.14
        cmp.s  := "hello"
        cmp.v3 := float3(1, 2, 3)
        cmp.v4 := float4(1, 2, 3, 4)
        cmp.b  := true
        cmp.u  := 0xdeadbeefu
    }
    commit()
    query(eid) $(i : int; f : float; s : string; v3 : float3; v4 : float4; b : bool; u : uint) {
        t |> equal(i, 42)
        t |> equal(f, 3.14)
        t |> equal(s, "hello")
        t |> equal(v3, float3(1, 2, 3))
        t |> equal(v4, float4(1, 2, 3, 4))
        t |> equal(b, true)
        t |> equal(u, 0xdeadbeefu)
    }
}

[test]
def test_entity_count_across_archetypes(t : T?) {
    restart()
    // 3 entities with {pos}
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp.pos := float3(i)
        }
    }
    // 2 entities with {pos, vel}
    for (i in range(2)) {
        create_entity() @(eid, cmp) {
            cmp.pos := float3(i)
            cmp.vel := float3(1)
        }
    }
    commit()
    var total = 0
    query() $(pos : float3) {
        total++
    }
    t |> equal(total, 5)
    var velCount = 0
    query() $(vel : float3) {
        velCount++
    }
    t |> equal(velCount, 2)
}
