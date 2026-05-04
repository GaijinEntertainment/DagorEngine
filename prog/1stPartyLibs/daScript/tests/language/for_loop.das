// Tests for-loop mechanics: fixed arrays, dynamic arrays, ranges, urange, range64, multi-variable
options gen2
require dastest/testing_boost public

def test_from_gte_to(t : T?) {
    for (x in range(3, 2)) {
        t |> failure("loop body should not execute")
    }
    for (x in urange(3, 2)) {
        t |> failure("loop body should not execute")
    }
    for (x in range(3, 2)) {
        t |> failure("loop body should not execute")
    }
    for (x in urange(3, 2)) {
        t |> failure("loop body should not execute")
    }
    for (x in range(3, 2)) {
        break
    }
    for (x in urange(3, 2)) {
        break
    }
    for (x in range(3, 2)) {
        t |> failure("loop body should not execute")
        break
    }
    for (x in urange(3, 2)) {
        t |> failure("loop body should not execute")
        break
    }
    for (x, y in range(3, 2), urange(3, 2)) {
        t |> failure("loop body should not execute")
    }
    for (x, y in urange(3, 2), urange(3, 2)) {
        t |> failure("loop body should not execute")
    }
    for (x in each(range(3, 2))) {
        t |> failure("loop body should not execute")
    }
}

def test_for64(t : T?) {
    {
        var val = 0l
        for (x, y in range64(100), count()) {
            t |> equal(val, x)
            t |> equal(val, int64(y))
            val ++
        }
        t |> equal(val, 100l)
    }
    {
        var val = 0ul
        for (x in urange64(100)) {
            t |> equal(val, x)
            val ++
        }
        t |> equal(val, 100ul)
    }
}

[test]
def test_for_loop(t : T?) {
    t |> run("1 variable fixed array") @(t : T?) {
        var a : int[10]
        var i = 0
        for (l in a) {
            l = i
            i++
        }
        t |> equal(i, 10)
        i = 0
        for (l in a) {
            t |> equal(l, i)
            i++
        }
    }
    t |> run("1 variable dynamic array") @(t : T?) {
        var a : array<int>
        var i = 0
        resize(a, 13)
        for (l in a) {
            l = i++
        }
        t |> equal(i, 13)
        i = 0
        for (l in a) {
            t |> equal(l, i)
            i++
        }
    }
    t |> run("2 variables fixed array") @(t : T?) {
        var a, b : int[10]
        var i = 0
        for (l, r in a, b) {
            r = i * 2 - 5
            l = i++
        }
        t |> equal(i, 10)
        i = 0
        for (l, r in a, b) {
            t |> equal(r, i * 2 - 5)
            t |> equal(l, i)
            i++
        }
        i = 0
        for (l, r in a, b) {
            if (l > r) {
                i++
            }
        }
        t |> equal(i, 5)
    }
    t |> run("2 variables hybrid array") @(t : T?) {
        var a : array<int>
        var b : int[10]
        var i = 0
        resize(a, 4)
        for (l, r in a, b) {
            t |> success(i < 4)
            r = i * 2 - 5
            l = i++
        }
        t |> equal(i, 4)
        i = 0
        for (l, r in a, b) {
            t |> equal(r, i * 2 - 5)
            t |> equal(l, i)
            i++
        }
        t |> equal(i, 4)
        i = 0
        for (l, r in a, b) {
            if (l > r) {
                i++
            }
        }
        t |> equal(i, 4)
    }
    t |> run("1 variable range") @(t : T?) {
        var i = 0
        for (a in range(10)) {
            t |> equal(a, i)
            i++
        }
        t |> equal(i, 10)
    }
    t |> run("2 variables range hybrid") @(t : T?) {
        var b : int[10]
        var i = 0
        for (l, r in range(10), b) {
            r = i++ * 2 - 5
        }
        t |> equal(i, 10)
        i = 0
        for (l, r in range(10), b) {
            t |> equal(r, i * 2 - 5)
            t |> equal(l, i)
            i++
        }
        i = 0
        for (l, r in range(10), b) {
            if (l > r) {
                i++
            }
        }
        t |> equal(i, 5)
    }
    t |> run("urange") @(t : T?) {
        var u = 0u
        for (tt in urange(0u, 3u)) {
            u += tt
        }
        t |> equal(u, 3u)
    }
    t |> run("reverse range (from >= to)") @(t : T?) {
        test_from_gte_to(t)
    }
    t |> run("64-bit ranges") @(t : T?) {
        test_for64(t)
    }
}
