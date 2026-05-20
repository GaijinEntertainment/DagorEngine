// Per-iteration finally on for-over-multi-source loops: `for (a, b in src1, src2)`.
options gen2
require dastest/testing_boost public

[test]
def test_for_multi_source(t : T?) {
    t |> run("plain") @(t : T?) {
        var count = 0
        var sum = 0
        for (a, b in range(4), range(10, 14)) {
            sum += a + b
        } finally {
            count ++
        }
        t |> equal(count, 4)
        t |> equal(sum, (0 + 1 + 2 + 3) + (10 + 11 + 12 + 13))
    }
    t |> run("break") @(t : T?) {
        var count = 0
        for (a, _b in range(10), range(100, 110)) {
            if (a == 3) {
                break
            }
        } finally {
            count ++
        }
        // iterations for a=0..3 each fire finally (break runs after a==3's finally)
        t |> equal(count, 4)
    }
    t |> run("continue") @(t : T?) {
        var count = 0
        var hits = 0
        for (a, _b in range(4), range(10, 14)) {
            if (a % 2 == 0) {
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
        t |> equal(for_multi_return(), 2)
    }
    t |> run("inscope") @(t : T?) {
        for (a, b in range(3), range(100, 103)) {
            var inscope tmp : array<int>
            tmp |> push(a); tmp |> push(b)
            t |> equal(tmp[0], a)
            t |> equal(tmp[1], b)
        }
    }
    t |> run("empty") @(t : T?) {
        var count = 0
        // shortest source is empty -> loop body never enters
        for (_a, _b in range(0), range(10, 15)) {
        } finally {
            count ++
        }
        t |> equal(count, 0)
    }
    t |> run("uneven sources") @(t : T?) {
        var count = 0
        // shortest source (length 3) determines iteration count
        for (_a, _b in range(10), range(100, 103)) {
        } finally {
            count ++
        }
        t |> equal(count, 3)
    }
}

def for_multi_return() : int {
    var count = 0
    for (a, _b in range(10), range(100, 110)) {
        if (a == 2) {
            return count
        }
    } finally {
        count ++
    }
    return -1
}
