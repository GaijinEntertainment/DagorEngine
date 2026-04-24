options gen2
options persistent_heap
options gc

require daslib/decs_boost
require dastest/testing_boost public
require daslib/random
require daslib/math
require daslib/static_let

[test]
def test_archive_arrays(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp |> set("values", [for (x in range(i + 1)); x])
            cmp |> set("i", i)
        }
    }
    commit()
    t |> run("before gc") @(t : T?) {
        var tot = 0
        query() $(values : array<int>; i : int) {
            tot ++
            for (x, v in range(i + 1), values) {
                t |> equal(x, v)
            }
        }
        t |> equal(tot, 3)
    }
    var data <- mem_archive_save(decsState)
    restart()
    data |> mem_archive_load(decsState)
    t |> run("after gc") @(t : T?) {
        var tot = 0
        query() $(values : array<int>; i : int) {
            tot ++
            for (x, v in range(i + 1), values) {
                t |> equal(x, v)
            }
        }
        t |> equal(tot, 3)
    }
}

[test]
def test_archive_strings(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp.txt := "hello_{i}"
        }
    }
    commit()
    t |> run("before string gc") @(t : T?) {
        var i = 0
        query() $(txt : string) {
            t |> equal(txt, "hello_{i++}")
        }
        t |> equal(i, 3)
    }
    var data <- mem_archive_save(decsState)
    restart()
    data |> mem_archive_load(decsState)
    t |> run("after string gc") @(t : T?) {
        var j = 0
        query() $(txt : string) {
            t |> equal(txt, "hello_{j++}")
        }
        t |> equal(j, 3)
    }
}

[test]
def test_gc_arrays(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp |> set("values", [for (x in range(i + 1)); x])
            cmp |> set("i", i)
        }
    }
    commit()
    t |> run("before gc") @(t : T?) {
        query() $(values : array<int>; i : int) {
            for (x, v in range(i + 1), values) {
                t |> equal(x, v)
            }
        }
    }
    before_gc()
    unsafe {
        heap_collect(true, true)
    }
    after_gc()
    t |> run("after gc") @(t : T?) {
        query() $(values : array<int>; i : int) {
            for (x, v in range(i + 1), values) {
                t |> equal(x, v)
            }
        }
    }
}

[test]
def test_gc_strings(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp.txt := "hello_{i}"
        }
    }
    commit()
    t |> run("before string gc") @(t : T?) {
        var i = 0
        query() $(txt : string) {
            t |> equal(txt, "hello_{i++}")
        }
        t |> equal(i, 3)
    }
    before_gc()
    unsafe {
        heap_collect(true, true)
    }
    after_gc()
    t |> run("after string gc") @(t : T?) {
        var j = 0
        query() $(txt : string) {
            t |> equal(txt, "hello_{j++}")
        }
        t |> equal(j, 3)
    }
}

var total_foo = 0
class FooWithLifetime {
    def FooWithLifetime {
        total_foo ++
    }
    def operator delete {
        total_foo --
    }
}

[test]
def test_arrays_lifetime(t : T?) {
    restart()
    for (i in range(3)) {
        create_entity() @(eid, cmp) {
            cmp |> set("pfoo", new FooWithLifetime())
        }
    }
    commit()
    t |> success(total_foo == 3)
    restart()
    t |> success(total_foo == 0)
}

[test]
def test_gc_stress(t : T?) {
    var seed = random_seed(13)
    restart()
    static_let() {
        var TOTAL_EID = 0
    }
    for (count in range(5)) {
        let morei = (random_int(seed) & 31) + 10
        for (i in range(morei)) {
            create_entity() @(eid, cmp) {
                var values <- [for (x in range(i)); int2(x, x + 13)]
                cmp |> set("values", values)
                let random_arch = random_int(seed) & 31
                cmp |> set("arch_{random_arch}", random_arch)
                cmp.i := i
                TOTAL_EID ++
            }
        }
        let morej = (random_int(seed) & 31) + 10
        for (j in range(morei)) {
            create_entity() @(eid, cmp) {
                cmp.txt := "text_{eid}"
                let random_arch = random_int(seed) & 63
                cmp |> set("arch_{random_arch}", random_arch)
                TOTAL_EID ++
            }
        }
        let delk = (random_int(seed) & 63)
        query() $(eid : EntityId) {
            let chance = random_int(seed) % 100
            if (chance <= 13) {
                delete_entity(eid)
                TOTAL_EID --
            }
        }
        if ((random_int(seed) % 100) < 50) {
            before_gc()
            unsafe {
                heap_collect(true, true)
            }
            after_gc()
        }
        commit()
        query() $(eid : EntityId; text : string) {
            t |> equal(text, "text_{eid}")
        }
        query() $(var values : array<int2>; var i : int) {
            let addi = (random_int(seed) & 511) + 10
            for (tt in range(addi)) {
                values |> push(int2(i + tt, i + tt + 13))
            }
            i += addi
        }
        query() $(var values : array<int2>; var i : int) {
            var subi = (random_int(seed) & 63) + 5
            subi = min(length(values) - 10, subi)
            for (tt in range(subi)) {
                values |> pop
            }
            i -= subi
        }
        query() $(values : array<int2>; i : int) {
            for (x, v in range(i + 1), values) {
                if (!t |> equal(int2(x, x + 13), v)) {
                    break
                }
            }
        }

        let r = random_int(seed) % 3
        if (r == 0) {
            pass
        } elif (r == 1) {
            var data <- mem_archive_save(decsState)
            restart()
            data |> mem_archive_load(decsState)
        } else {
            before_gc()
            unsafe {
                heap_collect(true, true)
            }
            after_gc()
        }
        query() $(values : array<int2>; i : int) {
            for (x, v in range(i + 1), values) {
                if (!t |> equal(int2(x, x + 13), v)) {
                    break
                }
            }
        }
        var te = 0
        query() $(eid : EntityId) {
            te ++
        }
        t |> equal(te, TOTAL_EID)
        te = 0
        find_query() $(eid : EntityId) {
            te ++
            return false
        }
        t |> equal(TOTAL_EID, te)
    }
}

