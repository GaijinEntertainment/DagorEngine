options gen2
options persistent_heap = true
options gc
options relaxed_pointer_const

require daslib/decs_boost
require dastest/testing_boost public
require daslib/static_let

[test]
def test_queries(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp |> set("pos", float3(1 + i, 2 + i, 3 + i))
            cmp |> set("vel", float3(1, 1, 1))
            cmp |> set("i", i)
        }
    }
    commit()
    query() $(pos : float3; vel : float3; i : int) {// check if we set the right values
        t |> equal(pos, float3(1 + i, 2 + i, 3 + i))
        t |> equal(vel, float3(1, 1, 1))
    }
    t |> run("nested") @(t : T?) {
        query() $(pos : float3; i : int) {
            let spos = pos
            let si = i
            query() $(pos : float3; i : int) {
                if (spos == pos) {
                    t |> equal(si, i)                  // if position is the same, index should be the same
                }
            }
        }
    }
    t |> run("nested query one") @(t : T?) {
        query() $(i : int; eid : EntityId) {
            let si = i
            query(eid) $(i : int) {// this query does lookup by eid
                t |> equal(si, i)                      //  we check if it can find the same thing
            }
        }
    }
    t |> run("nested with break") @(t : T?) {
        query() $(i : int) {
            for (j in range(0, i)) {
                if (j % 3 == 0) {
                    break                           // break is allowed, but only inside the loops - not query itself
                }
            }
        }
    }
    t |> run("nested with continue") @(t : T?) {
        query() $(i : int; var pos : float3) {
            for (j in range(0, i)) {
                if (j % 3 == 0) {
                    continue                        // continue is only allowed inside the loop, same as break
                }
                pos.x = pos.x
            }
        }
    }
}


[test]
def test_contracts(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp.pos := float3(i)
            cmp.vel := float3(.1, .2, .3)
        }
    }
    for (i in range(2)) {
        create_entity() @(eid, cmp) {
            cmp.pos := float3(i + 3)
            cmp.vel := float3(.1, .2, .3)
            cmp.col := 0xffffffff
        }
    }
    commit()
    t |> run("require not") @(t : T?) {
        var count_nc = 0
        query <| $ [REQUIRE_NOT(col)] (var pos : float3&) {
            pos = float3(13)
            count_nc ++
        }
        t |> equal(count_nc, 3)
    }
    t |> run("require") @(t : T?) {
        var count_c = 0
        query <| $ [REQUIRE(col)] (var pos : float3&) {
            pos = float3(13)
            count_c ++
        }
        t |> equal(count_c, 2)
    }
}

[test]
def test_find_query(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp.pos := float3(i)
        }
    }
    commit()
    t |> run("find specific") @(t : T?) {
        var looked_at = 0
        find_query() $(pos : float3) {
            looked_at ++
            return pos.x == 1.
        }
        t |> equal(looked_at, 2)
    }
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp.pos := float3(i)
            cmp.vel := float3(i)
        }
    }
    commit()
    t |> run("check look count") @(t : T?) {
        var looked_at = 0
        find_query() $(pos : float3) {
            looked_at ++
            return false
        }
        t |> equal(looked_at, 6)
    }
}

[test]
def test_query_eid(t : T?) {
    restart()
    let EID = create_entity() @(eid, cmp) {
        cmp.pos := float3(1)
    }
    commit()
    var count = 0
    query(EID) $(pos : float3) {
        t |> equal(pos, float3(1))
        count ++
    }
    t |> equal(count, 1)
}

[decs_template]
struct Something {
    a : int
}

[test]
def test_query_eid_obj(t : T?) {
    restart()
    let EID = create_entity() @(eid, cmp) {
        cmp |> apply_decs_template(Something(a = 1))
    }
    commit()
    var count = 0
    query(EID) $(s : Something) {
        t |> equal(s.a, 1)
        count ++
    }
    t |> equal(count, 1)
}

[test]
def test_query_everything(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp.i := 1
    }
    create_entity() @(eid, cmp) {
        cmp.j := 1
    }
    create_entity() @(eid, cmp) {
        cmp.i := 2
        cmp.j := 2
    }
    commit()
    query() $(@optional i : int?; j : int = 5; k : int = 5) {
        t |> equal(k, 5)    // not in any archetype, always default
        if (i == null) {
            t |> equal(j, 1)
        } elif (*i == 2) {
            t |> equal(j, 2)
        } else {
            t |> equal(j, 5)
        }
    }
}

[test]
def test_aka_query(t : T?) {
    restart()
    for (i in range(10)) {
        create_entity() @(eid, cmp) {
            cmp.i := i
        }
    }
    commit()
    var tot = 0
    var equ = 0
    query() $(i aka j : int) {
        query() $(i : int) {
            tot ++
            if (i == j) {
                equ++
            }
        }
    }
    t |> equal(tot, 100)
    t |> equal(equ, 10)
}


