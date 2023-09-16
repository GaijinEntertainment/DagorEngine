require math

[sideeffects]
def test_mat(m:float3x4)
    var t : float3x4 = m
    for x in range(3)
        for y in range(4)
            assert(t[y][x] == float(y*3 + x))
            assert(t[y][x] == m[y][x])

[sideeffects]
def test_transpose
    var a, b : float4x4
    for x in range(4)
        for y in range(4)
            a[x][y] = float(x*4+y)
            b[x][y] = float((x+1)*4+(y+1))
    let ta = transpose(a)
    for x in range(4)
        for y in range(4)
            assert(ta[y][x]==float(x*4+y))
    let tta = transpose(transpose(a))               // folds to a
    assert(tta==a)
    let tb = transpose(b)
    let tatb = ta * tb
    let tatb_opt = transpose(a) * transpose(b)      // folds to transpose(b*a)
    assert(tatb==tatb_opt)
    return true

[export]
def test
    var m : float3x4
    for x in range(3)
        for y in range(4)
            m[y][x] = float(y*3 + x)
    test_mat(m)
    test_transpose()
    return true

