options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/decs_boost
require math

// === is_alive ===

[test]
def test_is_alive_basic(t : T?) {
    t |> run("alive entity returns true") @(t : T?) {
        restart()
        var eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("name", "test")
        }
        commit()
        t |> equal(is_alive(eid), true)
        restart()
    }
}

[test]
def test_is_alive_invalid(t : T?) {
    t |> run("INVALID_ENTITY_ID is not alive") @(t : T?) {
        restart()
        t |> equal(is_alive(INVALID_ENTITY_ID), false)
        restart()
    }
}

[test]
def test_is_alive_deleted(t : T?) {
    t |> run("deleted entity is not alive") @(t : T?) {
        restart()
        var eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("hp", 100)
        }
        commit()
        t |> equal(is_alive(eid), true)
        delete_entity(eid)
        commit()
        t |> equal(is_alive(eid), false)
        restart()
    }
}

[test]
def test_is_alive_stale_generation(t : T?) {
    t |> run("stale generation returns false after slot recycling") @(t : T?) {
        restart()
        var old_eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("tag", "old")
        }
        commit()
        delete_entity(old_eid)
        commit()
        // Recycle the slot
        var new_eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("tag", "new")
        }
        commit()
        // Same id, different generation
        t |> equal(old_eid.id, new_eid.id)
        t |> equal(is_alive(old_eid), false)
        t |> equal(is_alive(new_eid), true)
        restart()
    }
}

[test]
def test_is_alive_out_of_bounds(t : T?) {
    t |> run("entity id beyond lookup table is not alive") @(t : T?) {
        restart()
        var fake_eid = EntityId(id = 99999u, generation = 1)
        t |> equal(is_alive(fake_eid), false)
        restart()
    }
}

[test]
def test_is_alive_multiple_entities(t : T?) {
    t |> run("mixed alive and dead entities") @(t : T?) {
        restart()
        var e1 = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("v", 1)
        }
        var e2 = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("v", 2)
        }
        var e3 = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("v", 3)
        }
        commit()
        delete_entity(e2)
        commit()
        t |> equal(is_alive(e1), true)
        t |> equal(is_alive(e2), false)
        t |> equal(is_alive(e3), true)
        restart()
    }
}

[test]
def test_is_alive_before_commit(t : T?) {
    t |> run("entity not alive before first commit") @(t : T?) {
        restart()
        var eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("x", 1)
        }
        // Entity is deferred — not yet in the lookup table with matching generation
        t |> equal(is_alive(eid), false)
        commit()
        t |> equal(is_alive(eid), true)
        restart()
    }
}

// === entity_count ===

[test]
def test_entity_count_empty(t : T?) {
    t |> run("empty world has zero entities") @(t : T?) {
        restart()
        t |> equal(entity_count(), 0)
        restart()
    }
}

[test]
def test_entity_count_basic(t : T?) {
    t |> run("counts entities across archetypes") @(t : T?) {
        restart()
        // Archetype 1: name only
        create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("name", "a")
        }
        // Archetype 2: name + hp
        create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("name", "b")
            cmp |> set("hp", 50)
        }
        // Archetype 3: hp only
        create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("hp", 100)
        }
        commit()
        t |> equal(entity_count(), 3)
        restart()
    }
}

[test]
def test_entity_count_after_delete(t : T?) {
    t |> run("count decreases after deletion") @(t : T?) {
        restart()
        var e1 = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("v", 1)
        }
        create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("v", 2)
        }
        create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("v", 3)
        }
        commit()
        t |> equal(entity_count(), 3)
        delete_entity(e1)
        commit()
        t |> equal(entity_count(), 2)
        restart()
    }
}

[test]
def test_entity_count_create_delete_same_commit(t : T?) {
    t |> run("create and delete in same commit") @(t : T?) {
        restart()
        create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("v", 1)
        }
        commit()
        t |> equal(entity_count(), 1)
        var e2 = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("v", 2)
        }
        delete_entity(e2)
        commit()
        // Entity was created then immediately deleted
        t |> equal(entity_count(), 1)
        restart()
    }
}

[test]
def test_entity_count_many(t : T?) {
    t |> run("counts 100 entities") @(t : T?) {
        restart()
        for (i in range(100)) {
            create_entity() @(eid : EntityId; var cmp : ComponentMap) {
                cmp |> set("idx", i)
            }
        }
        commit()
        t |> equal(entity_count(), 100)
        restart()
    }
}

// === get_component ===

[test]
def test_get_component_int(t : T?) {
    t |> run("get int component") @(t : T?) {
        restart()
        var eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("hp", 42)
        }
        commit()
        t |> equal(get_component(eid, "hp", 0), 42)
        restart()
    }
}

[test]
def test_get_component_float(t : T?) {
    t |> run("get float component") @(t : T?) {
        restart()
        var eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("speed", 3.14f)
        }
        commit()
        let result = get_component(eid, "speed", 0.0f)
        t |> equal(abs(result - 3.14f) < 0.01f, true)
        restart()
    }
}

[test]
def test_get_component_string(t : T?) {
    t |> run("get string component") @(t : T?) {
        restart()
        var eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("name", "hero")
        }
        commit()
        t |> equal(get_component(eid, "name", ""), "hero")
        restart()
    }
}

[test]
def test_get_component_float3(t : T?) {
    t |> run("get float3 component") @(t : T?) {
        restart()
        var eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("pos", float3(1.0, 2.0, 3.0))
        }
        commit()
        let result = get_component(eid, "pos", float3(0.0))
        t |> equal(abs(result.x - 1.0f) < 0.01f, true)
        t |> equal(abs(result.y - 2.0f) < 0.01f, true)
        t |> equal(abs(result.z - 3.0f) < 0.01f, true)
        restart()
    }
}

[test]
def test_get_component_bool(t : T?) {
    t |> run("get bool component") @(t : T?) {
        restart()
        var eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("active", true)
        }
        commit()
        t |> equal(get_component(eid, "active", false), true)
        restart()
    }
}

[test]
def test_get_component_missing(t : T?) {
    t |> run("missing component returns default") @(t : T?) {
        restart()
        var eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("hp", 100)
        }
        commit()
        t |> equal(get_component(eid, "nonexistent", -1), -1)
        restart()
    }
}

[test]
def test_get_component_dead_entity(t : T?) {
    t |> run("dead entity returns default") @(t : T?) {
        restart()
        var eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("hp", 100)
        }
        commit()
        delete_entity(eid)
        commit()
        t |> equal(get_component(eid, "hp", -999), -999)
        restart()
    }
}

[test]
def test_get_component_invalid_entity(t : T?) {
    t |> run("INVALID_ENTITY_ID returns default") @(t : T?) {
        restart()
        t |> equal(get_component(INVALID_ENTITY_ID, "hp", -1), -1)
        restart()
    }
}

[test]
def test_get_component_after_update(t : T?) {
    t |> run("reflects updated component value") @(t : T?) {
        restart()
        var eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("hp", 100)
        }
        commit()
        t |> equal(get_component(eid, "hp", 0), 100)
        update_entity(eid) @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("hp", 50)
        }
        commit()
        t |> equal(get_component(eid, "hp", 0), 50)
        restart()
    }
}

[test]
def test_get_component_stale_generation(t : T?) {
    t |> run("stale entity id returns default after recycling") @(t : T?) {
        restart()
        var old_eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("hp", 100)
        }
        commit()
        delete_entity(old_eid)
        commit()
        var new_eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("hp", 999)
        }
        commit()
        // Old eid should not see new entity's data
        t |> equal(get_component(old_eid, "hp", -1), -1)
        // New eid should work fine
        t |> equal(get_component(new_eid, "hp", 0), 999)
        restart()
    }
}

[test]
def test_get_component_multiple_entities(t : T?) {
    t |> run("get components from different entities") @(t : T?) {
        restart()
        var e1 = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("hp", 10)
            cmp |> set("name", "one")
        }
        var e2 = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("hp", 20)
            cmp |> set("name", "two")
        }
        commit()
        t |> equal(get_component(e1, "hp", 0), 10)
        t |> equal(get_component(e2, "hp", 0), 20)
        t |> equal(get_component(e1, "name", ""), "one")
        t |> equal(get_component(e2, "name", ""), "two")
        restart()
    }
}

[test]
def test_get_component_returns_copy(t : T?) {
    t |> run("returned value is a copy, not a reference") @(t : T?) {
        restart()
        var eid = create_entity() @(eid : EntityId; var cmp : ComponentMap) {
            cmp |> set("hp", 100)
        }
        commit()
        var val = get_component(eid, "hp", 0)
        val = 999
        // Original entity should be unaffected
        t |> equal(get_component(eid, "hp", 0), 100)
        restart()
    }
}
