options gen2
require math
require dastest/testing_boost public

def verifyIdentity(t : T?; mat : auto(matT)) {
    let v : auto(vec) = mat[0]
    let col = typeinfo sizeof(type<vec>) / 4
    let row = typeinfo sizeof(type<matT>) / col / 4
    for (y in range(row)) {
        for (x in range(col)) {
            let expected = x == y ? 1.0 : 0.0
            t |> equal(mat[y][x], expected)
        }
    }
}

def verifySequence(t : T?; mat : auto(matT)) {
    let v : auto(vec) = mat[0]
    let col = typeinfo sizeof(type<vec>) / 4
    let row = typeinfo sizeof(type<matT>) / col / 4
    var val = 1.0
    for (y in range(row)) {
        for (x in range(col)) {
            t |> equal(mat[y][x], val)
            val += 1.0
        }
    }
}

def verifyRows(t : T?; mat : auto(matT)) {
    let v : auto(vec) = mat[0]
    let col = typeinfo sizeof(type<vec>) / 4
    let row = typeinfo sizeof(type<matT>) / col / 4
    for (y in range(row)) {
        for (x in range(col)) {
            t |> equal(mat[y][x], float(y))
        }
    }
}

def test_4x4(t : T?) {
    var m : float4x4
    m[0] = float4(1, 2, 3, 4)
    m[1] = float4(5, 6, 7, 8)
    m[2] = float4(9, 10, 11, 12)
    m[3] = float4(13, 14, 15, 16)
    t |> equal("{m}", "( 1{VEC_SEP}2{VEC_SEP}3{VEC_SEP}4, 5{VEC_SEP}6{VEC_SEP}7{VEC_SEP}8, 9{VEC_SEP}10{VEC_SEP}11{VEC_SEP}12, 13{VEC_SEP}14{VEC_SEP}15{VEC_SEP}16)")
    verifySequence(t, m)
    for (y in range(4)) {
        for (x in range(4)) {
            m[y][x] = x == y ? 1.0 : 0.0
        }
    }
    verifyIdentity(t, m)
    m.x = float4(1, 2, 3, 4)
    m.y = float4(5, 6, 7, 8)
    m.z = float4(9, 10, 11, 12)
    m.w = float4(13, 14, 15, 16)
    verifySequence(t, m)
    t |> equal("{m}", "( 1{VEC_SEP}2{VEC_SEP}3{VEC_SEP}4, 5{VEC_SEP}6{VEC_SEP}7{VEC_SEP}8, 9{VEC_SEP}10{VEC_SEP}11{VEC_SEP}12, 13{VEC_SEP}14{VEC_SEP}15{VEC_SEP}16)")
    identity(m)
    verifyIdentity(t, m)
    var ops = false
    try {
        m[passx(4)] = float4(0)  // the reason to use pass here is so that we don't get compile time error
    } recover {
        ops = true
    }
    t |> success(ops)
}

[sideeffects]
def passx(a) {
    return a
}

def test_any(t : T?; var m : auto(matT)) {
    let v : auto(vec) = m[0]
    let col = typeinfo sizeof(type<vec>) / 4
    let row = typeinfo sizeof(type<matT>) / col / 4
    identity(m)
    verifyIdentity(t, m)
    for (r in range(row)) {
        m[r] = vec(r)
    }
    verifyRows(t, m)
    for (y in range(row)) {
        for (x in range(col)) {
            m[y][x] = x == y ? 1.0 : 0.0
        }
    }
    verifyIdentity(t, m)
    var ops = false
    try {
        m[passx(row)] = vec(0)   // the reason to use pass here is so that we don't get compile time error
    } recover {
        ops = true
    }
    t |> success(ops)
}

[sideeffects, no_jit] // assert-verify warning caused by wrong effects of jit index access function
def test_r2v(t : T?) {
    var m : float4x4
    m[0] = float4(1, 2, 3, 4)
    m[1].y = 6.
    m[2] = float4(5, 6, 7, 8)
    m.w = float4(9, 10, 11, 12)
    m.w.z = 111.
    t |> equal(m[0], float4(1, 2, 3, 4))
    t |> equal(m[1].y, 6.)
    t |> equal(m[1], float4(0, 6, 0, 0))
    t |> equal(m[2], float4(5, 6, 7, 8))
    t |> equal(m.w.z, 111.)
    t |> equal(m.w, float4(9, 10, 111, 12))
}

[test]
def test_mat_ctors(t : T?) {
    test_r2v(t)
    test_4x4(t)
    test_any(t, float4x4())
    test_any(t, float3x4())
}
