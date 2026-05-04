options gen2
options persistent_heap = true
options gc

require daslib/decs_boost
require dastest/testing_boost public

[test]
def test_arrays(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp |> set("values", [for (x in range(i + 1)); x])
            cmp |> set("i", i)
        }
    }
    commit()
    t |> run("iterate") @@(t : T?) {
        query() $(values : array<int>; i : int) {
            t |> equal(length(values), i + 1)
            for (x, v in range(i + 1), values) {
                t |> equal(x, v)
            }
        }
    }
    t |> run("modify") @@(t : T?) {
        query() $(var values : array<int>; i : int) {
            delete values
            for (j in range(5)) {
                values |> push(j + i)
            }
        }
    }
    t |> run("verify") @@(t : T?) {
        query() $(values : array<int>; i : int) {
            t |> equal(length(values), 5)
            for (j in range(5)) {
                t |> equal(values[j], i + j)
            }
        }
    }
}



