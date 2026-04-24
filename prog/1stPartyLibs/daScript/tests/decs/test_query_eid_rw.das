options gen2
options persistent_heap = true
options gc

require daslib/decs_boost
require dastest/testing_boost public

[decs_template]
struct CmpOne {
    num : int
}

[decs_template]
struct CmpTwo {
    num : int
}

[test]
def test_qeid_rw(t : T?) {
    create_entity() @(eid, cmp) {
        cmp |> set("eid", eid)
        apply_decs_template(cmp,  CmpOne())
        apply_decs_template(cmp,  CmpTwo())
    }
    commit()
    query() $(var c1 : CmpOne; eid : EntityId) {
        c1.num++
        query(eid) $(var c2 : CmpTwo) {
            c2.num++
        }
        query(eid) $(c2 : CmpTwo) {
            t |> equal(c1.num, 1)
            t |> equal(c2.num, 1)
        }
    }
}

