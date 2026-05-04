options gen2
options persistent_heap = true
options gc
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require daslib/decs_boost
require dastest/testing_boost public

// === Update component value ===

[test]
def test_update_modify_value(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.hp := 100
    }
    commit()
    update_entity(eid) @(eid, cmp) {
        var hp = 0
        hp = get(cmp, "hp", hp)
        cmp |> set("hp", hp - 25)
    }
    commit()
    var v = 0
    query(eid) $(hp : int) {
        v = hp
    }
    t |> equal(v, 75)
}

// === Update add component (archetype change) ===

[test]
def test_update_add_component(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.x := 1
    }
    commit()
    update_entity(eid) @(eid, cmp) {
        cmp.y := 2
    }
    commit()
    var found = false
    query(eid) $(x : int; y : int) {
        t |> equal(x, 1)
        t |> equal(y, 2)
        found = true
    }
    t |> success(found)
}

// === Update remove component (archetype change) ===

[test]
def test_update_remove_component(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.x := 10
        cmp.y := 20
    }
    commit()
    update_entity(eid) @(eid, cmp) {
        cmp |> remove("y")
    }
    commit()
    // y no longer present
    var foundXY = false
    query(eid) $(x : int; y : int) {
        foundXY = true
    }
    t |> success(!foundXY)
    // x still present
    var foundX = false
    query(eid) $(x : int) {
        t |> equal(x, 10)
        foundX = true
    }
    t |> success(foundX)
}

// === Update same archetype (no archetype change) ===

[test]
def test_update_same_archetype(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.val := 1
    }
    commit()
    let archCountBefore = length(decsState.allArchetypes)
    update_entity(eid) @(eid, cmp) {
        cmp |> set("val", 999)
    }
    commit()
    // No new archetype should be created
    t |> equal(length(decsState.allArchetypes), archCountBefore)
    var v = 0
    query(eid) $(val : int) {
        v = val
    }
    t |> equal(v, 999)
}

// === Update deleted entity is a no-op ===

[test]
def test_update_deleted_entity(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.val := 42
    }
    commit()
    delete_entity(eid)
    commit()
    // update on deleted entity should be a no-op
    update_entity(eid) @(eid, cmp) {
        cmp |> set("val", 999)
    }
    commit()
    var count = 0
    query() $(val : int) {
        count++
    }
    t |> equal(count, 0)
}

// === Multiple updates before commit ===

[test]
def test_multiple_updates_before_commit(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.val := 0
    }
    commit()
    // Queue two updates
    update_entity(eid) @(eid, cmp) {
        var v = 0
        v = get(cmp, "val", v)
        cmp |> set("val", v + 10)
    }
    update_entity(eid) @(eid, cmp) {
        var v = 0
        v = get(cmp, "val", v)
        cmp |> set("val", v + 5)
    }
    commit()
    var v = 0
    query(eid) $(val : int) {
        v = val
    }
    t |> equal(v, 15)
}

// === Update preserves other entities ===

[test]
def test_update_preserves_others(t : T?) {
    restart()
    let e1 = create_entity() @(eid, cmp) {
        cmp.val := 1
    }
    let e2 = create_entity() @(eid, cmp) {
        cmp.val := 2
    }
    commit()
    // update only e1 with new component
    update_entity(e1) @(eid, cmp) {
        cmp.extra := 100
    }
    commit()
    // e2 should be unaffected
    var v = 0
    query(e2) $(val : int) {
        v = val
    }
    t |> equal(v, 2)
    // e1 should have both
    var found = false
    query(e1) $(val : int; extra : int) {
        t |> equal(val, 1)
        t |> equal(extra, 100)
        found = true
    }
    t |> success(found)
}

// === Update with replace of component type (add + remove) ===

[test]
def test_update_replace_component(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.x := 1
        cmp.tag := "old"
    }
    commit()
    // remove label, add score
    update_entity(eid) @(eid, cmp) {
        cmp |> remove("tag")
        cmp.score := 99
    }
    commit()
    var found = false
    query(eid) $(x : int; score : int) {
        t |> equal(x, 1)
        t |> equal(score, 99)
        found = true
    }
    t |> success(found)
    // tag gone
    var foundTag = false
    query(eid) $(tag : string) {
        foundTag = true
    }
    t |> success(!foundTag)
}
