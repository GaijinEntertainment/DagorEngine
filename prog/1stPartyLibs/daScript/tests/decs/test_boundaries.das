options gen2
options persistent_heap = true
options gc

require daslib/decs_boost
require dastest/testing_boost public

[test]
def test_boundaries(t : T?) {
    restart()
    create_entity() @(eid, cmp) {
        cmp |> set("i", 1)
    }
    commit()
    t |> run("commit inside query") @@(t : T?) {
        query() $(i : int) {
            var failed = false
            try {
                commit()
            } recover {
                failed = true
            }
            t |> success(failed)
        }
    }
    t |> run("restart inside query") @@(t : T?) {
        query() $(i : int) {
            var failed = false
            try {
                commit()
            } recover {
                failed = true
            }
            t |> success(failed)
        }
    }
}


