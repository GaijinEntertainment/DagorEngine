// Stage 0 characterization for the tFixedArray rework (FIXED_ARRAY_REWORK.md).
// Pins iteration: for-loops at every depth, mutation through the loop ref,
// each(), parallel iteration with other sources.
options gen2
require dastest/testing_boost public

struct Cell {
    v : int
}

[test]
def test_for_loops(t : T?) {
    t |> run("1D for") @(t : T?) {
        var a : int[4]
        for (i in range(4)) {
            a[i] = i + 1
        }
        var sum = 0
        for (x in a) {
            sum += x
        }
        t |> equal(sum, 10)
    }
    t |> run("2D for yields rows") @(t : T?) {
        var m : int[2][3]
        for (y in range(2)) {
            for (x in range(3)) {
                m[y][x] = y * 3 + x
            }
        }
        var rows = 0
        var total = 0
        for (row in m) {
            rows ++
            for (x in row) {
                total += x
            }
        }
        t |> equal(rows, 2)
        t |> equal(total, 15)
    }
    t |> run("mutation through loop ref") @(t : T?) {
        var a : int[4]
        for (x in a) {
            x = 7
        }
        var sum = 0
        for (x in a) {
            sum += x
        }
        t |> equal(sum, 28)
    }
    t |> run("iterate FA of structs by ref") @(t : T?) {
        var cells : Cell[3]
        for (c in cells) {
            c.v = 5
        }
        t |> equal(cells[0].v + cells[1].v + cells[2].v, 15)
    }
}

[test]
def test_each_and_parallel(t : T?) {
    t |> run("each() over 1D") @(t : T?) {
        var s : int[4]
        for (i in range(4)) {
            s[i] = i * 10
        }
        var sum = 0
        for (v in each(s)) {
            sum += v
        }
        t |> equal(sum, 60)
    }
    t |> run("parallel iteration FA + range") @(t : T?) {
        var a : int[4]
        for (x, i in a, range(4)) {
            x = i * 2
        }
        t |> equal(a[3], 6)
    }
    t |> run("parallel iteration FA + array") @(t : T?) {
        var a : int[3]
        var b <- [10, 20, 30]
        var sum = 0
        for (x, y in a, b) {
            sum += x + y
        }
        t |> equal(sum, 60)
        delete b
    }
}
