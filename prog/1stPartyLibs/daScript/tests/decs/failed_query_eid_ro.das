options gen2
expect 30303

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
        apply_decs_template(cmp, CmpOne())
        apply_decs_template(cmp, CmpTwo())
    }
    commit()
    query() $(var c1 : CmpOne; eid : EntityId) {
        c1.num++
        query(eid) $(c2 : CmpTwo) {
            c2.num++                            // 30303: +++ can't be applied to constant int const&
        }
    }
}
