// Per-iteration finally on for-over-fixed-array (dim) loops.
options gen2
require dastest/testing_boost public

[test]
def test_for_fixed_array(t : T?) {
    t |> run("plain") @(t : T?) {
        var arr : int[5]
        arr[0] = 10; arr[1] = 20; arr[2] = 30; arr[3] = 40; arr[4] = 50
        var count = 0
        for (_x in arr) {
        } finally {
            count ++
        }
        t |> equal(count, 5)
    }
    t |> run("break") @(t : T?) {
        var arr : int[5]
        arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4; arr[4] = 5
        var count = 0
        for (x in arr) {
            if (x == 3) {
                break
            }
        } finally {
            count ++
        }
        t |> equal(count, 3)
    }
    t |> run("continue") @(t : T?) {
        var arr : int[4]
        arr[0] = 1; arr[1] = 2; arr[2] = 3; arr[3] = 4
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
        t |> equal(for_fixed_return(), 1)
    }
    t |> run("inscope") @(t : T?) {
        var src : int[3]
        src[0] = 7; src[1] = 8; src[2] = 9
        for (i in src) {
            var inscope tmp : array<int>
            tmp |> push(i)
            t |> equal(tmp[0], i)
        }
    }
}

def for_fixed_return() : int {
    var arr : int[3]
    arr[0] = 10; arr[1] = 20; arr[2] = 30
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
