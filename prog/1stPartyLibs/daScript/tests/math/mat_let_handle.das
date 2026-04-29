options gen2
require math
require dastest/testing_boost public

[sideeffects]
def test_mat(t : T?; m : float3x4) {
    var mat : float3x4 = m
    for (x in range(3)) {
        for (y in range(4)) {
            t |> equal(mat[y][x], float(y * 3 + x))
            t |> equal(mat[y][x], m[y][x])
        }
    }
}

[sideeffects]
def test_transpose(t : T?) {
    var a, b : float4x4
    for (x in range(4)) {
        for (y in range(4)) {
            a[x][y] = float(x * 4 + y)
            b[x][y] = float((x + 1) * 4 + (y + 1))
        }
    }
    let ta = transpose(a)
    for (x in range(4)) {
        for (y in range(4)) {
            t |> equal(ta[y][x], float(x * 4 + y))
        }
    }
    let tta = transpose(transpose(a))               // folds to a
    t |> equal(tta, a)
    let tb = transpose(b)
    let tatb = ta * tb
    let tatb_opt = transpose(a) * transpose(b)      // folds to transpose(b*a)
    t |> equal(tatb, tatb_opt)
}

[test]
def test_mat_let_handle(t : T?) {
    var m : float3x4
    for (x in range(3)) {
        for (y in range(4)) {
            m[y][x] = float(y * 3 + x)
        }
    }
    test_mat(t, m)
    test_transpose(t)
}
