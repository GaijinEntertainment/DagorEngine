options gen2
options persistent_heap = true
options gc
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require daslib/decs_boost
require dastest/testing_boost public

// === Deferred semantics: entities not visible until commit ===

[test]
def test_entity_not_visible_before_commit(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp.val := 42
    }
    // query before commit should find nothing
    var count = 0
    query() $(val : int) {
        count++
    }
    t |> equal(count, 0)
    commit()
    // now it should be visible
    query() $(val : int) {
        count++
    }
    t |> equal(count, 1)
}

[test]
def test_delete_not_effective_before_commit(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.val := 1
    }
    commit()
    delete_entity(eid)
    // entity still visible until commit
    var found = false
    query(eid) $(val : int) {
        found = true
    }
    t |> success(found)
    commit()
    // now gone
    found = false
    query(eid) $(val : int) {
        found = true
    }
    t |> success(!found)
}

// === Multiple operations in single commit ===

[test]
def test_batch_create_commit(t : T?) {
    restart()
    for (i in range(10)) {
        create_entity() @(eid, cmp) {
            cmp.idx := i
        }
    }
    commit()
    var count = 0
    query() $(idx : int) {
        count++
    }
    t |> equal(count, 10)
}

[test]
def test_create_and_delete_same_commit(t : T?) {
    restart()
    let e1 = create_entity() @(eid, cmp) {
        cmp.val := 1
    }
    commit()
    // create new + delete old in same batch
    let e2 = create_entity() @(eid, cmp) {
        cmp.val := 2
    }
    delete_entity(e1)
    commit()
    var vals : array<int>
    query() $(val : int) {
        vals |> push(val)
    }
    t |> equal(length(vals), 1)
    t |> equal(vals[0], 2)
}

[test]
def test_create_update_delete_same_commit(t : T?) {
    restart()
    let e1 = create_entity() @(eid, cmp) {
        cmp.val := 1
    }
    commit()
    // update + delete in same commit — delete should win (it's processed after update)
    update_entity(e1) @(eid, cmp) {
        cmp.val := 999
    }
    delete_entity(e1)
    commit()
    var count = 0
    query() $(val : int) {
        count++
    }
    t |> equal(count, 0)
}

// === Multiple sequential commits ===

[test]
def test_sequential_commits(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp.step := 1
    }
    commit()
    var count = 0
    query() $(step : int) {
        count++
    }
    t |> equal(count, 1)

    create_entity() @(eid, cmp) {
        cmp.step := 2
    }
    commit()
    count = 0
    query() $(step : int) {
        count++
    }
    t |> equal(count, 2)

    create_entity() @(eid, cmp) {
        cmp.step := 3
    }
    commit()
    count = 0
    query() $(step : int) {
        count++
    }
    t |> equal(count, 3)
}

// === Empty commit is a no-op ===

[test]
def test_empty_commit(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp.val := 42
    }
    commit()
    // extra commits do nothing
    commit()
    commit()
    var count = 0
    query() $(val : int) {
        count++
    }
    t |> equal(count, 1)
}

// === Deferred order: actions processed in order ===

[test]
def test_deferred_order(t : T?) {
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
    // verify order: e1=1, e2=2, e3=3
    var v = 0
    query(e1) $(val : int) {
        v = val
    }
    t |> equal(v, 1)
    query(e2) $(val : int) {
        v = val
    }
    t |> equal(v, 2)
    query(e3) $(val : int) {
        v = val
    }
    t |> equal(v, 3)
}
