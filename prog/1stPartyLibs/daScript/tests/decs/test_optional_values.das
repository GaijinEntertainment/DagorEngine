options gen2
options persistent_heap = true
options gc
options relaxed_pointer_const

require daslib/decs_boost
require dastest/testing_boost public

[test]
def test_optional_values(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp.a := 1
    }
    create_entity() @(eid, cmp) {
        cmp.a := 2
        cmp.b := 10
    }
    commit()
    t |> run("proper optional") @@(t : T?) {
        var count = 0
        query() $(a : int; @optional b : int?) {
            if (b == null) {
                t |> equal(a, 1)
            } else {
                t |> equal(a, 2)
                t |> equal(*b, 10)
            }
            count ++
        }
        t |> equal(count, 2)
    }
    t |> run("proper optional") @@(t : T?) {
        var failed = false
        try {
            // runtime error - component array b type mismatch, expecting int vs int?
            query() $(a : int; b : int?) {// note no optional
                pass
            }
        } recover {
            failed = true
        }
        t |> success(failed)
    }
}
