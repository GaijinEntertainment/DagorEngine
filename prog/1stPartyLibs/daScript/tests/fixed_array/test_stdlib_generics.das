// Stage 4 of the tFixedArray rework (FIXED_ARRAY_REWORK.md): multi-dim and
// typedef'd fixed arrays flow through the kept C-array-subject generics
// (each/sort/find_index/subarray/clone_dim/finalize_dim) via natural nesting.
options gen2
require dastest/testing_boost public

typedef M4 = float[4][4]

[test]
def test_each_multi_dim(t : T?) {
    t |> run("each over multi-dim yields rows") @(t : T?) {
        var m : float[4][4]
        for (i in range(4)) {
            m[i][0] = float(i * 10)
        }
        var rowsum = 0.0
        for (row in m) {
            rowsum += row[0]
        }
        t |> equal(rowsum, 60.0)
    }
    t |> run("each over typedef'd M4[N] yields M4 elements") @(t : T?) {
        var mm : M4[2]
        mm[1][2][3] = 42.0
        var found = 0
        for (one_m in mm) {
            if (one_m[2][3] == 42.0) {
                found ++
            }
        }
        t |> equal(found, 1)
    }
}

[test]
def test_sort_rows(t : T?) {
    t |> run("sort with cmp sorts whole rows") @(t : T?) {
        var s : int[3][2]
        s[0][0] = 30
        s[0][1] = 31
        s[1][0] = 10
        s[1][1] = 11
        s[2][0] = 20
        s[2][1] = 21
        s |> sort() $(x, y : int[2]) {
            return x[0] < y[0]
        }
        // rows move as units - the second column rides along
        t |> equal(s[0][0], 10)
        t |> equal(s[0][1], 11)
        t |> equal(s[1][0], 20)
        t |> equal(s[1][1], 21)
        t |> equal(s[2][0], 30)
        t |> equal(s[2][1], 31)
    }
}

[test]
def test_find_subarray_rows(t : T?) {
    t |> run("find_index 1-D") @(t : T?) {
        var f1 : int[4]
        f1[2] = 7
        t |> equal(find_index(f1, 7), 2)
    }
    t |> run("find_index_if over rows") @(t : T?) {
        var s : int[3][2]
        s[2][0] = 30
        let fi = find_index_if(s) $(row : int[2]) {
            return row[0] == 30
        }
        t |> equal(fi, 2)
    }
    t |> run("subarray over rows keeps the row type") @(t : T?) {
        var s : int[3][2]
        s[1][0] = 15
        s[2][0] = 25
        var sub <- subarray(s, range(1, 3))
        t |> equal(length(sub), 2)
        t |> equal(sub[0][0], 15)
        t |> equal(sub[1][0], 25)
        t |> equal(typeinfo typename(sub), "array<int[2]>")
        delete sub
    }
}

[test]
def test_clone_finalize_multi_dim(t : T?) {
    t |> run("clone_dim multi-dim") @(t : T?) {
        var src : float[4][4]
        src[3][0] = 30.0
        var dst : float[4][4]
        clone_dim(dst, src)
        t |> equal(dst[3][0], 30.0)
    }
    t |> run("finalize_dim over array rows") @(t : T?) {
        var rows : array<int>[2]
        rows[0] |> push(1)
        rows[1] |> push(2)
        finalize_dim(rows)
        t |> equal(length(rows[0]), 0)
        t |> equal(length(rows[1]), 0)
    }
}
