// Per-iteration finally on for-over-range loops.
options gen2
require dastest/testing_boost public

[test]
def test_for_range(t : T?) {
    t |> run("plain") @(t : T?) {
        var count = 0
        for (_x in range(10)) {
        } finally {
            count ++
        }
        t |> equal(count, 10)
    }
    t |> run("break") @(t : T?) {
        var count = 0
        for (x in range(10)) {
            if (x == 5) {
                break
            }
        } finally {
            count ++
        }
        // iterations 0..5 each fire finally (break after x==5's finally)
        t |> equal(count, 6)
    }
    t |> run("continue") @(t : T?) {
        var count = 0
        var hits = 0
        for (x in range(10)) {
            if (x % 2 == 0) {
                continue
            }
            hits ++
        } finally {
            count ++
        }
        t |> equal(count, 10)
        t |> equal(hits, 5)
    }
    t |> run("return") @(t : T?) {
        t |> equal(for_range_return(), 3)
    }
    t |> run("nested") @(t : T?) {
        var outer = 0
        var inner = 0
        for (_y in range(4)) {
            for (_x in range(3)) {
            } finally {
                inner ++
            }
        } finally {
            outer ++
        }
        t |> equal(outer, 4)
        t |> equal(inner, 12)
    }
    t |> run("inscope") @(t : T?) {
        for (i in range(5)) {
            var inscope arr : array<int>
            arr |> push(i)
            t |> equal(arr[0], i)
        }
    }
    t |> run("empty") @(t : T?) {
        var count = 0
        for (_x in range(0)) {
        } finally {
            count ++
        }
        // empty iterator: body never enters, finally never runs
        t |> equal(count, 0)
    }
}

def for_range_return() : int {
    var count = 0
    for (x in range(10)) {
        if (x == 3) {
            return count
        }
    } finally {
        count ++
    }
    return -1
}
