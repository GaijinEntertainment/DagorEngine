// Per-iteration finally on for-over-dynamic-array loops.
options gen2
require dastest/testing_boost public

[test]
def test_for_array(t : T?) {
    t |> run("plain") @(t : T?) {
        var arr <- [10, 20, 30]
        var count = 0
        for (_x in arr) {
        } finally {
            count ++
        }
        t |> equal(count, 3)
    }
    t |> run("break") @(t : T?) {
        var arr <- [1, 2, 3, 4, 5]
        var count = 0
        for (x in arr) {
            if (x == 3) {
                break
            }
        } finally {
            count ++
        }
        // iterations for 1,2,3 each fire finally (break runs after x==3's finally)
        t |> equal(count, 3)
    }
    t |> run("continue") @(t : T?) {
        var arr <- [1, 2, 3, 4]
        var count = 0
        var hits = 0
        for (x in arr) {
            if (x % 2 == 0) {
                continue
            }
            hits ++
        } finally {
            count ++
        }
        t |> equal(count, 4)
        t |> equal(hits, 2)
    }
    t |> run("return") @(t : T?) {
        // return captures count=1 (after iter x=10's finally); iter x=20's finally runs after return expr is evaluated
        t |> equal(for_array_return(), 1)
    }
    t |> run("nested") @(t : T?) {
        var a <- [1, 2, 3]
        var b <- [10, 20]
        var outer = 0
        var inner = 0
        for (_y in a) {
            for (_x in b) {
            } finally {
                inner ++
            }
        } finally {
            outer ++
        }
        t |> equal(outer, 3)
        t |> equal(inner, 6)
    }
    t |> run("inscope") @(t : T?) {
        var src <- [1, 2, 3]
        for (i in src) {
            var inscope tmp : array<int>
            tmp |> push(i); tmp |> push(i * 2)
            t |> equal(tmp[0], i)
            t |> equal(tmp[1], i * 2)
        }
    }
    t |> run("empty") @(t : T?) {
        var arr : array<int>
        var count = 0
        for (_x in arr) {
        } finally {
            count ++
        }
        // empty array: body never enters, finally never runs
        t |> equal(count, 0)
    }
}

def for_array_return() : int {
    var arr <- [10, 20, 30]
    var count = 0
    for (x in arr) {
        if (x == 20) {
            return count
        }
    } finally {
        count ++
    }
    return -1
}
