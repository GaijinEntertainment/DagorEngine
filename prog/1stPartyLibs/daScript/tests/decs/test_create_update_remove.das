options gen2
options persistent_heap = true
options gc

require daslib/decs_boost
require dastest/testing_boost public
require daslib/static_let
require daslib/apply
require daslib/strings_boost

[test]
def test_create_update_remove(t : T?) {
    restart()
    static_let() {
        var migr : EntityId
    }
    t |> run("create entity") @(t : T?) {
        create_entity() @(eid, cmp) {
            cmp.pos := float3(1, 2, 3)
            cmp.vel := float3(0., -9.8, 0.)
            verify(cmp |> has("pos"))
            verify(cmp |> has("vel"))
            migr = eid
        }
        commit()
        query(migr) $(pos, vel : float3) {
            assert(pos == float3(1, 2, 3))
            assert(vel == float3(0., -9.8, 0.))
        }
    }
    t |> run("update entity") @(t : T?) {
        migr |> update_entity() @(eid, cmp) {
            verify(cmp |> has("pos"))
            verify(cmp |> has("vel"))
            verify(!cmp |> has("color"))
            cmp.color := float4(1, 0, 0, 1)
            verify(cmp |> has("color"))
            migr = eid
        }
        commit()
        query(migr) $(pos, vel : float3; color : float4) {
            assert(pos == float3(1, 2, 3))
            assert(color == float4(1, 0, 0, 1))
            assert(vel == float3(0., -9.8, 0.))
        }
    }
    t |> run("modify entity") @(t : T?) {
        migr |> update_entity() @(eid, cmp) {
            verify(cmp |> has("vel"))
            cmp |> remove("vel")
            verify(!cmp |> has("vel"))
            migr = eid
        }
        commit()
        query(migr) $(pos : float3; color : float4) {
            assert(pos == float3(1, 2, 3))
            assert(color == float4(1, 0, 0, 1))
        }
    }
    t |> run("empty update") @(t : T?) {
        migr |> update_entity() @(eid, cmp) {
            migr = eid
        }
        commit()
    }
    t |> run("remove") @(t : T?) {
        migr |> delete_entity
        commit()
        query(migr) $(pos : float3; color : float4) {
            panic("somehwow did not remove")
        }
    }
}

[test]
def test_create_delete(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp.pos  := float3(1., 2., 3.)
        cmp.name := "entity_once"
    }
    static_let() {
        var de : array<EntityId>
    }
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            if (i == 1 || i == 2) {
                de |> push(eid)
            }
            cmp.pos  := float3(1 + i, 2 + i, 3 + i)
            cmp.name := "entity{i}"
            cmp.id   := i
        }
    }
    commit()
    t |> run("verify create") @(t : T?) {
        var total = 0
        query() $(name : string) {
            success(t, name |> starts_with("entity"))
            total ++
        }
        t |> equal(total, 4)
    }
    for (d in de) {
        delete_entity(d)
    }
    commit()
    t |> run("verify delete") @(t : T?) {
        var total = 0
        query() $(name : string) {
            if (name != "entity0" && name != "entity_once") {
                t->fatal("deleted something else {name}")
            }
            total ++
        }
        t |> equal(total, 2)
    }
}
