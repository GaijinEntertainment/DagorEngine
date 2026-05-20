// Per-iteration finally on while loops.
options gen2
require dastest/testing_boost public

[test]
def test_while(t : T?) {
    t |> run("plain") @(t : T?) {
        var i = 0
        var count = 0
        while (i < 5) {
            i ++
        } finally {
            count ++
        }
        t |> equal(count, 5)
    }
    t |> run("break") @(t : T?) {
        var i = 0
        var count = 0
        while (i < 10) {
            if (i == 3) {
                break
            }
            i ++
        } finally {
            count ++
        }
        // iterations i=0,1,2,3 each fire finally (break runs after i==3's finally)
        t |> equal(count, 4)
    }
    t |> run("continue") @(t : T?) {
        var i = 0
        var count = 0
        var hits = 0
        while (i < 6) {
            i ++
            if (i % 2 == 0) {
                continue
            }
            hits ++
        } finally {
            count ++
        }
        t |> equal(count, 6)
        t |> equal(hits, 3)
    }
    t |> run("return") @(t : T?) {
        t |> equal(while_return(), 2)
    }
    t |> run("nested") @(t : T?) {
        var outer_count = 0
        var inner_count = 0
        var y = 0
        while (y < 3) {
            var x = 0
            while (x < 4) {
                x ++
            } finally {
                inner_count ++
            }
            y ++
        } finally {
            outer_count ++
        }
        t |> equal(outer_count, 3)
        t |> equal(inner_count, 12)
    }
    t |> run("inscope") @(t : T?) {
        var i = 0
        while (i < 5) {
            var inscope tmp : array<int>
            tmp |> push(i)
            t |> equal(tmp[0], i)
            i ++
        }
    }
    t |> run("never taken") @(t : T?) {
        var count = 0
        while (false) {
        } finally {
            count ++
        }
        // condition false from the start: body never enters, finally never runs
        t |> equal(count, 0)
    }
}

def while_return() : int {
    var i = 0
    var count = 0
    while (i < 10) {
        if (i == 2) {
            return count
        }
        i ++
    } finally {
        count ++
    }
    return -1
}
