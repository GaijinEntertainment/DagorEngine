// options log=true, log_stack=true, optimize=true
require math

def verifyIdentity(mat:auto(mat))
    let v : auto(vec) = mat[0]
    let col = typeinfo(sizeof type<vec>) / 4
    let row = typeinfo(sizeof type<mat>) / col / 4
    for y in range(row)
        for x in range(col)
            let t = x==y ? 1.0 : 0.0
            assert(mat[y][x]==t)

def verifySequence(mat:auto(mat))
    let v : auto(vec) = mat[0]
    let col = typeinfo(sizeof type<vec>) / 4
    let row = typeinfo(sizeof type<mat>) / col / 4
    var t = 1.0
    for y in range(row)
        for x in range(col)
            verify(mat[y][x]==t++)

def verifyRows(mat:auto(mat))
    let v : auto(vec) = mat[0]
    let col = typeinfo(sizeof type<vec>) / 4
    let row = typeinfo(sizeof type<mat>) / col / 4
    for y in range(row)
        for x in range(col)
            assert(mat[y][x]==float(y))

def test_4x4
    var t : float4x4
    t[0] = float4(1,2,3,4)
    t[1] = float4(5,6,7,8)
    t[2] = float4(9,10,11,12)
    t[3] = float4(13,14,15,16)
    assert("{t}"=="[[ 1{VEC_SEP}2{VEC_SEP}3{VEC_SEP}4; 5{VEC_SEP}6{VEC_SEP}7{VEC_SEP}8; 9{VEC_SEP}10{VEC_SEP}11{VEC_SEP}12; 13{VEC_SEP}14{VEC_SEP}15{VEC_SEP}16]]")
    verifySequence(t)
    for y in range(4)
        for x in range(4)
            t[y][x] = x==y ? 1.0 : 0.0
    verifyIdentity(t)
    t.x = float4(1,2,3,4)
    t.y = float4(5,6,7,8)
    t.z = float4(9,10,11,12)
    t.w = float4(13,14,15,16)
    verifySequence(t)
    assert("{t}"=="[[ 1{VEC_SEP}2{VEC_SEP}3{VEC_SEP}4; 5{VEC_SEP}6{VEC_SEP}7{VEC_SEP}8; 9{VEC_SEP}10{VEC_SEP}11{VEC_SEP}12; 13{VEC_SEP}14{VEC_SEP}15{VEC_SEP}16]]")
    identity(t)
    verifyIdentity(t)
    var ops = false
    try
        t[passx(4)] = float4(0)  // the reason to use pass here is so that we don't get compile time error
    recover
        ops = true
    assert(ops)

[sideeffects]
def passx(a)
    return a

def test_any(var t:auto(mat))
    let v : auto(vec) = t[0]
    let col = typeinfo(sizeof type<vec>) / 4
    let row = typeinfo(sizeof type<mat>) / col / 4
    identity(t)
    verifyIdentity(t)
    for r in range(row)
        t[r] = vec(r)
    verifyRows(t)
    for y in range(row)
        for x in range(col)
            t[y][x] = x==y ? 1.0 : 0.0
    verifyIdentity(t)
    var ops = false
    try
        t[passx(row)] = vec(0)   // the reason to use pass here is so that we don't get compile time error
    recover
        ops = true
    assert(ops)

[sideeffects]
def test_r2v
    var m : float4x4
    m[0] = float4(1,2,3,4)
    m[1].y = 6.
    m[2] = float4(5,6,7,8)
    m.w = float4(9,10,11,12)
    m.w.z = 111.
    assert(m[0]==float4(1,2,3,4))
    assert(m[1].y==6.)
    assert(m[1]==float4(0,6,0,0))
    assert(m[2]==float4(5,6,7,8))
    assert(m.w.z==111.)
    assert(m.w==float4(9,10,111,12))
    return true

[export]
def test
    test_r2v()
    test_4x4()
    test_any(float4x4())
    test_any(float3x4())
    return true
