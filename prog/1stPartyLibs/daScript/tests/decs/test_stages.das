options gen2
options persistent_heap = true
options gc

require daslib/decs_boost
require daslib/apply
require daslib/static_let

require dastest/testing_boost public

[test]
def test_baisc_stage(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp |> set("pos", float3(1 + i, 2 + i, 3 + i))
            cmp |> set("vel", float3(1, 1, 1))
            cmp |> set("i", i)
        }
    }
    commit()
    query() $(pos : float3; vel : float3; i : int) {
        t |> equal(pos, float3(1 + i, 2 + i, 3 + i))
        t |> equal(vel, float3(1, 1, 1))
    }

    t |> run("update") @(t : T?) {
        decs_stage("update")
        query() $(pos : float3; vel : float3; i : int) {
            t |> equal(pos, float3(1 + i, 2 + i, 3 + i) + vel)
        }
    }
}


[decs(stage = update)]
def update(var pos : float3&; vel : float3) {
    pos += vel
}

[decs(stage="make_visible")]
def make_visible(var visible : bool) {
    visible = true
}

[decs(stage="check_visible")]
def check_visible(visible : bool) {
    assert(visible)
}

require test_stages_extra

[test]
def test_two_stage(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp.visible := false
        }
    }
    commit()
    t |> run("make visible and check") @(t : T?) {
        decs_stage("make_visible")
        decs_stage("check_visible")
    }
    t |> run("check again") @(t : T?) {
        query() $(visible : bool) {
            t |> equal(visible, true)
        }
    }
    t |> run("make visible, which is in the other file") @(t : T?) {
        decs_stage("make_invisible")
    }
    t |> run("check one more time") @(t : T?) {
        query() $(visible : bool) {
            t |> equal(visible, false)
        }
    }
}
