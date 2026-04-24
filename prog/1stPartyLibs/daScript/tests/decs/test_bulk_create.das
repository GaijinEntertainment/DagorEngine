options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require daslib/decs_boost
require daslib/archive

[decs_template]
struct BulkItem {
    value : int
    pos   : float3
}

[test]
def test_bulk_create_basic(t : T?) {
    t |> run("creates correct number of entities") @(t : T?) {
        restart()
        create_entities(10) <| $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, BulkItem(value = i, pos = float3(float(i))))
        }
        var count = 0
        query <| $(b : BulkItem) {
            count++
        }
        t |> equal(count, 10)
    }

    t |> run("entities have correct values") @(t : T?) {
        restart()
        create_entities(5) <| $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, BulkItem(value = i * 10, pos = float3(float(i), 0.0, 0.0)))
        }
        var sum = 0
        query <| $(b : BulkItem) {
            sum += b.value
        }
        t |> equal(sum, 0 + 10 + 20 + 30 + 40)
    }

    t |> run("entities have valid eids") @(t : T?) {
        restart()
        create_entities(3) <| $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, BulkItem(value = i))
        }
        var alive_count = 0
        query <| $(eid : EntityId; b : BulkItem) {
            t |> success(is_alive(eid), "entity should be alive")
            alive_count++
        }
        t |> equal(alive_count, 3)
    }
}

[test]
def test_bulk_create_immediate(t : T?) {
    t |> run("entities visible without commit") @(t : T?) {
        restart()
        create_entities(5) <| $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, BulkItem(value = i))
        }
        // No commit() — entities should already be visible
        var count = 0
        query <| $(b : BulkItem) {
            count++
        }
        t |> equal(count, 5)
    }
}

[test]
def test_bulk_create_zero(t : T?) {
    t |> run("zero count is a no-op") @(t : T?) {
        restart()
        create_entities(0) <| $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, BulkItem(value = i))
        }
        var count = 0
        query <| $(b : BulkItem) {
            count++
        }
        t |> equal(count, 0)
    }
}

[test]
def test_bulk_create_mixed_with_regular(t : T?) {
    t |> run("bulk + regular entities coexist") @(t : T?) {
        restart()
        create_entities(3) <| $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, BulkItem(value = i))
        }
        create_entity <| @(eid, cmp) {
            apply_decs_template(cmp, BulkItem(value = 99))
        }
        commit()
        var count = 0
        var found_99 = false
        query <| $(b : BulkItem) {
            count++
            if (b.value == 99) {
                found_99 = true
            }
        }
        t |> equal(count, 4)
        t |> success(found_99, "regular entity should exist alongside bulk")
    }
}

[test]
def test_bulk_create_large(t : T?) {
    t |> run("1000 entities") @(t : T?) {
        restart()
        create_entities(1000) <| $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, BulkItem(value = i, pos = float3(float(i), 0.0, 0.0)))
        }
        var count = 0
        var sum = 0
        query <| $(b : BulkItem) {
            count++
            sum += b.value
        }
        t |> equal(count, 1000)
        t |> equal(sum, 999 * 500) // sum of 0..999
    }
}

[test]
def test_bulk_delete_after(t : T?) {
    t |> run("can delete bulk-created entities") @(t : T?) {
        restart()
        create_entities(5) <| $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, BulkItem(value = i))
        }
        // Delete entities with value >= 3
        query <| $(eid : EntityId; b : BulkItem) {
            if (b.value >= 3) {
                delete_entity(eid)
            }
        }
        commit()
        var count = 0
        query <| $(b : BulkItem) {
            count++
        }
        t |> equal(count, 3)
    }
}

// --- Direct archetype write tests (create_entities`T) ---

[test]
def test_direct_bulk_basic(t : T?) {
    t |> run("direct: creates correct number of entities") @(t : T?) {
        restart()
        create_entities`BulkItem(10) <| $(eid : EntityId; i : int; var entity : BulkItem) {
            entity.value = i
            entity.pos = float3(float(i))
        }
        var count = 0
        query <| $(b : BulkItem) {
            count++
        }
        t |> equal(count, 10)
    }
}

[test]
def test_direct_bulk_values(t : T?) {
    t |> run("direct: entities have correct values") @(t : T?) {
        restart()
        create_entities`BulkItem(5) <| $(eid : EntityId; i : int; var entity : BulkItem) {
            entity.value = i * 10
            entity.pos = float3(float(i), 0.0, 0.0)
        }
        var sum = 0
        query <| $(b : BulkItem) {
            sum += b.value
        }
        t |> equal(sum, 0 + 10 + 20 + 30 + 40)
    }
}

[test]
def test_direct_bulk_eids(t : T?) {
    t |> run("direct: entities have valid eids") @(t : T?) {
        restart()
        create_entities`BulkItem(3) <| $(eid : EntityId; i : int; var entity : BulkItem) {
            entity.value = i
        }
        var alive_count = 0
        query <| $(eid : EntityId; b : BulkItem) {
            t |> success(is_alive(eid), "entity should be alive")
            alive_count++
        }
        t |> equal(alive_count, 3)
    }
}

[test]
def test_direct_bulk_large(t : T?) {
    t |> run("direct: 1000 entities") @(t : T?) {
        restart()
        create_entities`BulkItem(1000) <| $(eid : EntityId; i : int; var entity : BulkItem) {
            entity.value = i
            entity.pos = float3(float(i), 0.0, 0.0)
        }
        var count = 0
        var sum = 0
        query <| $(b : BulkItem) {
            count++
            sum += b.value
        }
        t |> equal(count, 1000)
        t |> equal(sum, 999 * 500)
    }
}

[test]
def test_direct_bulk_zero(t : T?) {
    t |> run("direct: zero count is a no-op") @(t : T?) {
        restart()
        create_entities`BulkItem(0) <| $(eid : EntityId; i : int; var entity : BulkItem) {
            entity.value = i
        }
        var count = 0
        query <| $(b : BulkItem) {
            count++
        }
        t |> equal(count, 0)
    }
}

[test]
def test_direct_bulk_mixed_with_regular(t : T?) {
    t |> run("direct: coexists with regular entities") @(t : T?) {
        restart()
        create_entities`BulkItem(3) <| $(eid : EntityId; i : int; var entity : BulkItem) {
            entity.value = i
        }
        create_entity <| @(eid, cmp) {
            apply_decs_template(cmp, BulkItem(value = 77))
        }
        commit()
        var count = 0
        var found_77 = false
        query <| $(b : BulkItem) {
            count++
            if (b.value == 77) {
                found_77 = true
            }
        }
        t |> equal(count, 4)
        t |> success(found_77, "regular entity should exist alongside direct bulk")
    }
}

[test]
def test_direct_bulk_delete_after(t : T?) {
    t |> run("direct: can delete bulk-created entities") @(t : T?) {
        restart()
        create_entities`BulkItem(5) <| $(eid : EntityId; i : int; var entity : BulkItem) {
            entity.value = i
        }
        query <| $(eid : EntityId; b : BulkItem) {
            if (b.value >= 3) {
                delete_entity(eid)
            }
        }
        commit()
        var count = 0
        query <| $(b : BulkItem) {
            count++
        }
        t |> equal(count, 3)
    }
}

[test]
def test_direct_bulk_serialize(t : T?) {
    t |> run("direct: bulk-created entities survive serialization") @(t : T?) {
        restart()
        create_entities`BulkItem(5) <| $(eid : EntityId; i : int; var entity : BulkItem) {
            entity.value = i * 10
            entity.pos = float3(float(i), 0.0, 0.0)
        }
        var data <- mem_archive_save(decsState)
        restart()
        mem_archive_load(data, decsState)
        var count = 0
        var sum = 0
        query <| $(b : BulkItem) {
            count++
            sum += b.value
        }
        t |> equal(count, 5)
        t |> equal(sum, 0 + 10 + 20 + 30 + 40)
    }
}

[test]
def test_direct_bulk_query_mutable(t : T?) {
    t |> run("direct: mutable query on bulk-created entities") @(t : T?) {
        restart()
        create_entities`BulkItem(3) <| $(eid : EntityId; i : int; var entity : BulkItem) {
            entity.value = i
        }
        query <| $(var b : BulkItem) {
            b.value += 100
        }
        var sum = 0
        query <| $(b : BulkItem) {
            sum += b.value
        }
        t |> equal(sum, 100 + 101 + 102)
    }
}

[test]
def test_direct_bulk_multiple_batches(t : T?) {
    t |> run("direct: multiple bulk batches coexist") @(t : T?) {
        restart()
        create_entities`BulkItem(3) <| $(eid : EntityId; i : int; var entity : BulkItem) {
            entity.value = i
        }
        create_entities`BulkItem(4) <| $(eid : EntityId; i : int; var entity : BulkItem) {
            entity.value = 100 + i
        }
        var count = 0
        var sum = 0
        query <| $(b : BulkItem) {
            count++
            sum += b.value
        }
        t |> equal(count, 7)
        t |> equal(sum, 0 + 1 + 2 + 100 + 101 + 102 + 103)
    }
}

[test]
def test_bulk_serialize(t : T?) {
    t |> run("cmp-based bulk entities survive serialization") @(t : T?) {
        restart()
        create_entities(5) <| $(eid : EntityId; i : int; var cmp : ComponentMap) {
            apply_decs_template(cmp, BulkItem(value = i * 10, pos = float3(float(i), 0.0, 0.0)))
        }
        var data <- mem_archive_save(decsState)
        restart()
        mem_archive_load(data, decsState)
        var count = 0
        var sum = 0
        query <| $(b : BulkItem) {
            count++
            sum += b.value
        }
        t |> equal(count, 5)
        t |> equal(sum, 0 + 10 + 20 + 30 + 40)
    }
}
