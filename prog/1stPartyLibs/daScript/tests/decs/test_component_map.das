options gen2
options persistent_heap = true
options gc
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require daslib/decs_boost
require dastest/testing_boost public

// === ComponentMap set/get/has/remove ===

[test]
def test_component_map_set_get_int(t : T?) {
    var cmp : ComponentMap
    cmp |> set("x", 42)
    var v = 0
    v = get(cmp, "x", v)
    t |> equal(v, 42)
}

[test]
def test_component_map_set_get_float(t : T?) {
    var cmp : ComponentMap
    cmp |> set("y", 3.14)
    var v = 0.0
    v = get(cmp, "y", v)
    t |> equal(v, 3.14)
}

[test]
def test_component_map_set_get_string(t : T?) {
    var cmp : ComponentMap
    cmp |> set("name", "hello")
    var v = ""
    v = get(cmp, "name", v)
    t |> equal(v, "hello")
}

[test]
def test_component_map_has(t : T?) {
    var cmp : ComponentMap
    cmp |> set("alpha", 1)
    t |> success(cmp |> has("alpha"))
    t |> success(!(cmp |> has("beta")))
}

[test]
def test_component_map_remove(t : T?) {
    var cmp : ComponentMap
    cmp |> set("a", 10)
    cmp |> set("b", 20)
    t |> success(cmp |> has("a"))
    cmp |> remove("a")
    t |> success(!(cmp |> has("a")))
    t |> success(cmp |> has("b"))
}

[test]
def test_component_map_overwrite(t : T?) {
    var cmp : ComponentMap
    cmp |> set("x", 1)
    cmp |> set("x", 99)
    var v = 0
    v = get(cmp, "x", v)
    t |> equal(v, 99)
}

[test]
def test_component_map_dot_syntax(t : T?) {
    // cmp.name := value uses operator . + operator :=
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.score := 100
        cmp.tag := "test"
    }
    commit()
    var foundScore = 0
    var foundTag = ""
    query(eid) $(score : int; tag : string) {
        foundScore = score
        foundTag = tag
    }
    t |> equal(foundScore, 100)
    t |> equal(foundTag, "test")
}

[test]
def test_component_map_sorted_order(t : T?) {
    // Components in ComponentMap are kept in sorted order by name
    var cmp : ComponentMap
    cmp |> set("zebra", 1)
    cmp |> set("alpha", 2)
    cmp |> set("middle", 3)
    t |> equal(cmp[0].name, "alpha")
    t |> equal(cmp[1].name, "middle")
    t |> equal(cmp[2].name, "zebra")
}

[test]
def test_component_map_remove_nonexistent(t : T?) {
    // Removing a non-existent component should be a no-op
    var cmp : ComponentMap
    cmp |> set("x", 1)
    cmp |> remove("nonexistent")
    t |> success(cmp |> has("x"))
    t |> equal(length(cmp), 1)
}

// === Multiple component types ===

[test]
def test_wide_component_types(t : T?) {
    restart()
    let eid = create_entity() @(eid, cmp) {
        cmp.vi2  := int2(1, 2)
        cmp.vi3  := int3(3, 4, 5)
        cmp.vi4  := int4(6, 7, 8, 9)
        cmp.vf2  := float2(1.5, 2.5)
        cmp.vd   := 3.14lf
        cmp.vr   := range(10, 20)
        cmp.vb   := true
        cmp.vu8  := uint8(255)
        cmp.vi64 := int64(987654321)
    }
    commit()
    query(eid) $(vi2 : int2; vi3 : int3; vi4 : int4; vf2 : float2; vd : double; vr : range; vb : bool; vu8 : uint8; vi64 : int64) {
        t |> equal(vi2, int2(1, 2))
        t |> equal(vi3, int3(3, 4, 5))
        t |> equal(vi4, int4(6, 7, 8, 9))
        t |> equal(vf2, float2(1.5, 2.5))
        t |> equal(vd, 3.14lf)
        t |> equal(vr, range(10, 20))
        t |> equal(vb, true)
        t |> equal(vu8, uint8(255))
        t |> equal(vi64, int64(987654321))
    }
}

// === EntityId as component ===

[test]
def test_entity_id_component(t : T?) {
    restart()
    let parent = create_entity() @(eid, cmp) {
        cmp.name := "parent"
    }
    let child = create_entity() @(eid, cmp) {
        cmp.name := "child"
        cmp.parent_id := parent
    }
    commit()
    var foundParent = INVALID_ENTITY_ID
    query(child) $(parent_id : EntityId) {
        foundParent = parent_id
    }
    t |> equal(foundParent, parent)
}
