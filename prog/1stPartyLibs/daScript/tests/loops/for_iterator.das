// Per-iteration finally on for-over-iterator loops (SimNode_ForWithIterator path).
options gen2
require dastest/testing_boost public

def make_iter(n : int) : iterator<int> {
    return <- generator<int>() <| $() {
        for (i in range(n)) {
            yield i
        }
        return false
    }
}

[test]
def test_for_iterator(t : T?) {
    t |> run("plain") @(t : T?) {
        var count = 0
        for (_x in make_iter(5)) {
        } finally {
            count ++
        }
        t |> equal(count, 5)
    }
    t |> run("break") @(t : T?) {
        var count = 0
        for (x in make_iter(10)) {
            if (x == 4) {
                break
            }
        } finally {
            count ++
        }
        // iterations for x=0..4 each fire finally (break runs after x==4's finally)
        t |> equal(count, 5)
    }
    t |> run("continue") @(t : T?) {
        var count = 0
        var hits = 0
        for (x in make_iter(6)) {
            if (x % 2 == 0) {
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
        t |> equal(for_iterator_return(), 2)
    }
    t |> run("inscope") @(t : T?) {
        for (i in make_iter(4)) {
            var inscope tmp : array<int>
            tmp |> push(i); tmp |> push(i * 10)
            t |> equal(tmp[0], i)
            t |> equal(tmp[1], i * 10)
        }
    }
    t |> run("empty") @(t : T?) {
        var count = 0
        for (_x in make_iter(0)) {
        } finally {
            count ++
        }
        t |> equal(count, 0)
    }
}

def for_iterator_return() : int {
    var count = 0
    for (x in make_iter(5)) {
        if (x == 2) {
            return count
        }
    } finally {
        count ++
    }
    return -1
}
