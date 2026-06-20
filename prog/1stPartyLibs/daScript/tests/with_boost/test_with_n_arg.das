options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/with_boost

struct A {
    f1 : int
}

[test]
def test_five_arrays(t : T?) {
    //! 5 array containers, one block, all-array. Proves the inline
    //! emission scales past the old per-arity-helper limit (3).
    var a1 <- [A(f1 = 1)]
    var a2 <- [A(f1 = 2)]
    var a3 <- [A(f1 = 3)]
    var a4 <- [A(f1 = 4)]
    var a5 <- [A(f1 = 5)]
    with_(a1[0], a2[0], a3[0], a4[0], a5[0]) $(x1, x2, x3, x4, x5) {
        x1.f1 = x1.f1 + x2.f1 + x3.f1 + x4.f1 + x5.f1
    }
    t |> equal(a1[0].f1, 15, "1 + 2 + 3 + 4 + 5 = 15 written to a1[0]")
    t |> equal(a2[0].f1, 2, "a2 untouched")
    t |> equal(a5[0].f1, 5, "a5 untouched")
}

[test]
def test_mixed_4_arrays_1_table(t : T?) {
    //! 4 arrays + 1 table — single table allowed, mixed positionally.
    var arr_w <- [0]
    var arr_x <- [10]
    var arr_y <- [100]
    var arr_z <- [1000]
    var tab : table<string; int>
    tab |> insert("k", 50)
    with_(arr_w[0], arr_x[0], tab["k"], arr_y[0], arr_z[0]) $(w, x, t_k, y, z) {
        w = x + t_k + y + z
        t_k = 999
    }
    t |> equal(arr_w[0], 10 + 50 + 100 + 1000, "sum across mixed array+table args")
    t |> equal(tab["k"], 999, "table entry mutated")
}

[test]
def test_seven_arrays(t : T?) {
    //! Even larger N — verifies macro really doesn't have an arity cap.
    var a <- [1]
    var b <- [2]
    var c <- [3]
    var d <- [4]
    var e <- [5]
    var f <- [6]
    var g <- [7]
    with_(a[0], b[0], c[0], d[0], e[0], f[0], g[0]) $(va, vb, vc, vd, ve, vf, vg) {
        va = vb + vc + vd + ve + vf + vg
    }
    t |> equal(a[0], 27, "2+3+4+5+6+7 = 27")
}
