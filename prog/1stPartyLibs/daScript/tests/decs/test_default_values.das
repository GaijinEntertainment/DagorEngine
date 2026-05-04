options gen2
options persistent_heap = true
options gc

require daslib/decs_boost
require dastest/testing_boost public

[decs_template]
struct Particle {
    live = 13
}

[test]
def test_create_from_default(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        apply_decs_template(cmp, Particle())
    }
    commit()
    query() $(var p : Particle) {
        p.live ++
    }
    query() $(var p : Particle) {
        t |> equal(p.live, 14)
    }
}

[test]
def test_default_values(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp.a := 1
    }
    create_entity() @(eid, cmp) {
        cmp.a := 2
        cmp.b := 10
    }
    commit()
    var count = 0
    query() $(a : int; b : int = -1) {
        if (b == -1) {
            t |> equal(a, 1)
        } elif (b == 10) {
            t |> equal(a, 2)
        }
        count ++
    }
    t |> equal(count, 2)
}

