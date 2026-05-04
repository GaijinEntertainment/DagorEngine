options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

// Tests vector swizzle operations (.xy, .yx, .xyz, .wzyx, etc.)
// for float/int/uint 2/3/4 and struct member vectors.

struct Foo {
    t : bool
    v : int3 = int3(1, 2, 3)
}

def funV(v : int3) : int3 {
    return v
}

def test_vec2_swizzle(tt : T?; a : auto(vec2) -const -&) {
    var f2_x = vec2(1, 2)
    let x : auto(numT) = f2_x[0]
    tt |> equal("{f2_x}", "1{VEC_SEP}2")
    var f2_y = f2_x.yx
    tt |> equal("{f2_y}", "2{VEC_SEP}1")
    f2_x.x = numT(3)
    f2_x.y = numT(4)
    tt |> equal("{f2_x}", "3{VEC_SEP}4")
    f2_x.xy = vec2(5, 6)
    tt |> equal("{f2_x}", "5{VEC_SEP}6")
    f2_y = f2_x.xy
    tt |> equal("{f2_y}", "5{VEC_SEP}6")
}

def test_vec3_swizzle(tt : T?; a : auto(vec3) -const -&) {
    var f3_x = vec3(1, 2, 3)
    let x : auto(numT) = f3_x[0]
    let y : auto(vec2) = f3_x.xy
    tt |> equal("{f3_x}", "1{VEC_SEP}2{VEC_SEP}3")
    var f3_y = f3_x.zyx
    tt |> equal("{f3_y}", "3{VEC_SEP}2{VEC_SEP}1")
    f3_x.x = numT(3)
    f3_x.y = numT(4)
    f3_x.z = numT(5)
    tt |> equal("{f3_x}", "3{VEC_SEP}4{VEC_SEP}5")
    f3_x.xyz = vec3(5, 6, 7)
    tt |> equal("{f3_x}", "5{VEC_SEP}6{VEC_SEP}7")
    f3_y = f3_x.xyz
    tt |> equal("{f3_y}", "5{VEC_SEP}6{VEC_SEP}7")
    f3_x.xy = vec2(8, 9)
    tt |> equal("{f3_x}", "8{VEC_SEP}9{VEC_SEP}7")
    f3_x.yz = vec2(1, 2)
    tt |> equal("{f3_x}", "8{VEC_SEP}1{VEC_SEP}2")
    tt |> equal(f3_x.zx, vec2(2, 8))
}

def test_vec4_swizzle(tt : T?; a : auto(vec4) -const -&) {
    var f4_x = vec4(1, 2, 3, 4)
    let x : auto(numT) = f4_x[0]
    let y : auto(vec2) = f4_x.xy
    let z : auto(vec3) = f4_x.xyz
    tt |> equal("{f4_x}", "1{VEC_SEP}2{VEC_SEP}3{VEC_SEP}4")
    var f4_y = f4_x.wzyx
    tt |> equal("{f4_y}", "4{VEC_SEP}3{VEC_SEP}2{VEC_SEP}1")
    f4_x.x = numT(3)
    f4_x.y = numT(4)
    f4_x.z = numT(5)
    f4_x.w = numT(6)
    tt |> equal("{f4_x}", "3{VEC_SEP}4{VEC_SEP}5{VEC_SEP}6")
    f4_x.xyzw = vec4(5, 6, 7, 8)
    tt |> equal("{f4_x}", "5{VEC_SEP}6{VEC_SEP}7{VEC_SEP}8")
    f4_y = f4_x.xyzw
    tt |> equal("{f4_y}", "5{VEC_SEP}6{VEC_SEP}7{VEC_SEP}8")
    f4_x.xyz = vec3(8, 9, 0)
    tt |> equal("{f4_x}", "8{VEC_SEP}9{VEC_SEP}0{VEC_SEP}8")
    f4_x.yz = vec2(1, 2)
    tt |> equal("{f4_x}", "8{VEC_SEP}1{VEC_SEP}2{VEC_SEP}8")
    tt |> equal(f4_x.zx, vec2(2, 8))
}

[test]
def test_vec_swizzle(t : T?) {
    t |> run("float2/3/4 swizzle") @(t : T?) {
        test_vec2_swizzle(t, float2())
        test_vec3_swizzle(t, float3())
        test_vec4_swizzle(t, float4())
    }
    t |> run("int2/3/4 swizzle") @(t : T?) {
        test_vec2_swizzle(t, int2())
        test_vec3_swizzle(t, int3())
        test_vec4_swizzle(t, int4())
    }
    t |> run("uint2/3/4 swizzle") @(t : T?) {
        test_vec2_swizzle(t, uint2())
        test_vec3_swizzle(t, uint3())
        test_vec4_swizzle(t, uint4())
    }
    t |> run("int3 struct member swizzle") @(t : T?) {
        // regular variable
        var v = int3(1, 2, 3)
        t |> equal(v, int3(1, 2, 3))
        t |> equal(v.x, 1)
        t |> equal(v.y, 2)
        t |> equal(v.z, 3)
        t |> equal(v.xy, int2(1, 2))
        t |> equal(v.yz, int2(2, 3))
        t |> equal(v.zx, int2(3, 1))
        t |> equal(v.yx, int2(2, 1))
        t |> equal(v.zy, int2(3, 2))
        t |> equal(v.xz, int2(1, 3))
        t |> equal(funV(v), int3(1, 2, 3))
        t |> equal(funV(v).x, 1)
        v.x = 11
        t |> equal(v, int3(11, 2, 3))
        v.y = 22
        t |> equal(v, int3(11, 22, 3))
        v.z = 33
        t |> equal(v, int3(11, 22, 33))
        v.xy = int2(1, 2)
        t |> equal(v.x, 1)
        t |> equal(v.y, 2)
        t |> equal(v.z, 33)
        v.yz = int2(2, 3)
        t |> equal(v.z, 3)
        v.xyz = int3(11, 22, 33)
        t |> equal(v.xyz, int3(11, 22, 33))
    }
    t |> run("struct field swizzle") @(t : T?) {
        var fv = Foo()
        t |> equal(fv.v, int3(1, 2, 3))
        t |> equal(fv.v.x, 1)
        t |> equal(fv.v.y, 2)
        t |> equal(fv.v.z, 3)
        t |> equal(fv.v.xy, int2(1, 2))
        t |> equal(fv.v.yz, int2(2, 3))
        t |> equal(fv.v.zx, int2(3, 1))
        t |> equal(fv.v.yx, int2(2, 1))
        t |> equal(funV(fv.v), int3(1, 2, 3))
        t |> equal(funV(fv.v).x, 1)
        fv.v.x = 11
        t |> equal(fv.v, int3(11, 2, 3))
        fv.v.y = 22
        t |> equal(fv.v, int3(11, 22, 3))
        fv.v.z = 33
        t |> equal(fv.v, int3(11, 22, 33))
        fv.v.xy = int2(1, 2)
        t |> equal(fv.v.x, 1)
        t |> equal(fv.v.y, 2)
        t |> equal(fv.v.z, 33)
        fv.v.yz = int2(2, 3)
        t |> equal(fv.v.z, 3)
        fv.v.xyz = int3(11, 22, 33)
        t |> equal(fv.v.xyz, int3(11, 22, 33))
    }
}
