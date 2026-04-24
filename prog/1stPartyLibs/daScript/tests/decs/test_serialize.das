options gen2
options persistent_heap = true
options gc

require daslib/decs_boost
require dastest/testing_boost public

[test]
def test_serialize(var t : T?) {
    restart()
    let entitiesNum = 3
    for (i in range(entitiesNum)) {
        create_entity() @(eid, cmp) {
            cmp |> set("i", i)
        }
    }
    for (i in range(entitiesNum)) {
        create_entity() @(eid, cmp) {
            cmp |> set("i", i)
            cmp |> set("s", "__{i}__")
        }
    }
    commit()
    var check_it <- $() {
        var num = 0
        query() $(i : int) {
            num += 1
            if (i < 0 || i >= 3) {
                t->fatal("wrong size")
            }
        }
        t |> equal(num, entitiesNum + entitiesNum)
        num = 0
        query() $(i : int; s : string) {
            num += 1
            t |> equal(s, "__{i}__")
        }
        t |> equal(num, entitiesNum)
    }
    invoke(check_it)
    var data <- mem_archive_save(decsState)
    restart()
    query() $(i : int) {
        panic("something is up")
    }

    data |> mem_archive_load(decsState)
    invoke(check_it)
}
