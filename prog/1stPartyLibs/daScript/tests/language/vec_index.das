options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

// Tests vector indexing [], safe indexing ?[], out-of-range bounds checking,
// and array-of-vectors indexing for float/int/uint 2/3/4 and range types.

def test_vec2(tt : T?; a : auto(vec2) -const -&) {
    var f2_x = vec2()
    let x : auto(numT) = f2_x[0]
    tt |> equal(f2_x[0], numT(0))
    tt |> equal(f2_x[1], numT(0))
    tt |> equal(f2_x?[0] ?? numT(-1), numT(0))
    tt |> equal(f2_x?[1] ?? numT(-1), numT(0))
    f2_x = vec2(1, 2)
    tt |> equal("{f2_x}", "1{VEC_SEP}2")
    tt |> equal(f2_x[0], numT(1))
    tt |> equal(f2_x[1], numT(2))
    tt |> equal(f2_x[0], vec2(1, 2)[0])
    tt |> equal(f2_x[1], vec2(1, 2)[1])
    f2_x[0] = numT(2)
    f2_x[1] = numT(3)
    tt |> equal(f2_x[0], numT(2))
    tt |> equal(f2_x[1], numT(3))
    tt |> equal(f2_x?[0] ?? numT(-1), numT(2))
    tt |> equal(f2_x?[1] ?? numT(-1), numT(3))
    // out-of-range checks
    var oor = false
    try {
        f2_x[-1] = numT(5)
    } recover {
        oor = true
    }
    tt |> success(oor)
    oor = false
    try {
        f2_x[2] = numT(5)
    } recover {
        oor = true
    }
    tt |> success(oor)
    // array of vectors
    var v2_vec : array<vec2> = [vec2(numT(1), numT(2)), vec2(numT(4), numT(5))]
    tt |> equal(v2_vec[0][0], numT(1))
    tt |> equal(v2_vec[1][1], numT(5))
}

def test_vec3(tt : T?; a : auto(vec3) -const -&) {
    var f3_x = vec3()
    let x : auto(numT) = f3_x[0]
    tt |> equal(f3_x[0], numT(0))
    tt |> equal(f3_x[1], numT(0))
    tt |> equal(f3_x[2], numT(0))
    tt |> equal(f3_x?[0] ?? numT(-1), numT(0))
    tt |> equal(f3_x?[1] ?? numT(-1), numT(0))
    tt |> equal(f3_x?[2] ?? numT(-1), numT(0))
    f3_x = vec3(1, 2, 3)
    tt |> equal("{f3_x}", "1{VEC_SEP}2{VEC_SEP}3")
    tt |> equal(f3_x[0], numT(1))
    tt |> equal(f3_x[1], numT(2))
    tt |> equal(f3_x[2], numT(3))
    tt |> equal(f3_x[0], vec3(1, 2, 3)[0])
    tt |> equal(f3_x[1], vec3(1, 2, 3)[1])
    tt |> equal(f3_x[2], vec3(1, 2, 3)[2])
    f3_x[0] = numT(2)
    f3_x[1] = numT(3)
    f3_x[2] = numT(4)
    tt |> equal(f3_x[0], numT(2))
    tt |> equal(f3_x[1], numT(3))
    tt |> equal(f3_x[2], numT(4))
    tt |> equal(f3_x?[0] ?? numT(-1), numT(2))
    tt |> equal(f3_x?[1] ?? numT(-1), numT(3))
    tt |> equal(f3_x?[2] ?? numT(-1), numT(4))
    var oor = false
    try {
        f3_x[-1] = numT(5)
    } recover {
        oor = true
    }
    tt |> success(oor)
    oor = false
    try {
        f3_x[3] = numT(5)
    } recover {
        oor = true
    }
    tt |> success(oor)
}

def test_vec4(tt : T?; a : auto(vec4) -const -&) {
    var f4_x = vec4()
    let x : auto(numT) = f4_x[0]
    tt |> equal(f4_x[0], numT(0))
    tt |> equal(f4_x[1], numT(0))
    tt |> equal(f4_x[2], numT(0))
    tt |> equal(f4_x[3], numT(0))
    tt |> equal(f4_x?[0] ?? numT(-1), numT(0))
    tt |> equal(f4_x?[1] ?? numT(-1), numT(0))
    tt |> equal(f4_x?[2] ?? numT(-1), numT(0))
    tt |> equal(f4_x?[3] ?? numT(-1), numT(0))
    f4_x = vec4(1, 2, 3, 4)
    tt |> equal("{f4_x}", "1{VEC_SEP}2{VEC_SEP}3{VEC_SEP}4")
    tt |> equal(f4_x[0], numT(1))
    tt |> equal(f4_x[1], numT(2))
    tt |> equal(f4_x[2], numT(3))
    tt |> equal(f4_x[3], numT(4))
    tt |> equal(f4_x[0], vec4(1, 2, 3, 4)[0])
    tt |> equal(f4_x[1], vec4(1, 2, 3, 4)[1])
    tt |> equal(f4_x[2], vec4(1, 2, 3, 4)[2])
    tt |> equal(f4_x[3], vec4(1, 2, 3, 4)[3])
    f4_x[0] = numT(2)
    f4_x[1] = numT(3)
    f4_x[2] = numT(4)
    f4_x[3] = numT(5)
    tt |> equal(f4_x[0], numT(2))
    tt |> equal(f4_x[1], numT(3))
    tt |> equal(f4_x[2], numT(4))
    tt |> equal(f4_x[3], numT(5))
    tt |> equal(f4_x?[0] ?? numT(-1), numT(2))
    tt |> equal(f4_x?[1] ?? numT(-1), numT(3))
    tt |> equal(f4_x?[2] ?? numT(-1), numT(4))
    tt |> equal(f4_x?[3] ?? numT(-1), numT(5))
    var oor = false
    try {
        f4_x[-1] = numT(5)
    } recover {
        oor = true
    }
    tt |> success(oor)
    oor = false
    try {
        f4_x[4] = numT(5)
    } recover {
        oor = true
    }
    tt |> success(oor)
}

[test]
def test_vec_index(t : T?) {
    t |> run("float2/3/4 indexing") @(t : T?) {
        test_vec2(t, float2())
        test_vec3(t, float3())
        test_vec4(t, float4())
    }
    t |> run("int2/3/4 indexing") @(t : T?) {
        test_vec2(t, int2())
        test_vec3(t, int3())
        test_vec4(t, int4())
    }
    t |> run("uint2/3/4 indexing") @(t : T?) {
        test_vec2(t, uint2())
        test_vec3(t, uint3())
        test_vec4(t, uint4())
    }
    t |> run("range indexing") @(t : T?) {
        test_vec2(t, range())
        test_vec2(t, urange())
    }
}
