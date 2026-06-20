// Stage 0 characterization for the tFixedArray rework (FIXED_ARRAY_REWORK.md).
// Pins indexing: read/write at every depth, index expressions, const sources,
// range slicing via subarray, indexing through struct fields.
options gen2
require dastest/testing_boost public

struct Holder {
    grid : int[3][3]
}

let g_const : int[4] = fixed_array(10, 20, 30, 40)

[test]
def test_index_rw(t : T?) {
    t |> run("1D read/write") @(t : T?) {
        var a : int[4]
        for (i in range(4)) {
            a[i] = i * i
        }
        t |> equal(a[0], 0)
        t |> equal(a[3], 9)
    }
    t |> run("2D and 3D read/write") @(t : T?) {
        var m : int[2][3]
        for (y in range(2)) {
            for (x in range(3)) {
                m[y][x] = y * 10 + x
            }
        }
        t |> equal(m[1][2], 12)
        var c : int[2][2][2]
        c[1][0][1] = 42
        t |> equal(c[1][0][1], 42)
        t |> equal(c[0][0][0], 0)
    }
    t |> run("index with expressions") @(t : T?) {
        var a : int[8]
        for (i in range(8)) {
            a[i] = i
        }
        let k = 3
        t |> equal(a[k + 1], 4)
        t |> equal(a[k * 2], 6)
    }
    t |> run("partial index of 2D yields row") @(t : T?) {
        var m : float[2][3]
        m[1][0] = 7.0
        t |> equal(typeinfo typename(m[1]), "float[3]")
        let row = m[1]      // copies the row
        t |> equal(row[0], 7.0)
        m[1][0] = 8.0
        t |> equal(row[0], 7.0)
    }
}

[test]
def test_const_and_fields(t : T?) {
    t |> run("const global FA reads") @(t : T?) {
        t |> equal(g_const[0], 10)
        t |> equal(g_const[3], 40)
    }
    t |> run("indexing through struct field") @(t : T?) {
        var h : Holder
        h.grid[2][1] = 21
        t |> equal(h.grid[2][1], 21)
        t |> equal(h.grid[0][0], 0)
    }
}

[test]
def test_subarray(t : T?) {
    t |> run("subarray range view") @(t : T?) {
        var s : int[4]
        for (i in range(4)) {
            s[i] = i * 10
        }
        var got : array<int>
        for (v in subarray(s, range(1, 3))) {
            got |> push(v)
        }
        t |> equal(length(got), 2)
        t |> equal(got[0], 10)
        t |> equal(got[1], 20)
        delete got
    }
    t |> run("range-index sugar a[r]") @(t : T?) {
        var s : int[4]
        for (i in range(4)) {
            s[i] = i + 1
        }
        var sum = 0
        for (v in s[range(0, 2)]) {
            sum += v
        }
        t |> equal(sum, 3)
    }
}
