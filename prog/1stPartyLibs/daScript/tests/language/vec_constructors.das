options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

// Tests vector constructors for float2/3/4, int2/3/4, uint2/3/4, range types,
// and type conversions between vector types.
// VEC_SEP is a built-in constant (default ",") used in vector printing.

def test_vec2(tt : T?; a : auto(vec2) -const -&; isRange = false) {
    let f2_0  = vec2()
    let f2_x  = vec2(1.0, 2.0)
    let f2_y  = vec2(1, 2)
    let f2_z  = vec2(0x1, 0x2)
    let f2_w  = vec2(1u, 2u)
    let f2_xx = vec2(1.0)
    let f2_yy = vec2(1)
    let f2_zz = vec2(0x1)
    let f2_ww = vec2(1u)
    tt |> equal("{f2_0}", "0{VEC_SEP}0")
    tt |> equal("{f2_x}", "1{VEC_SEP}2")
    tt |> equal("{f2_y}", "1{VEC_SEP}2")
    tt |> equal("{f2_z}", "1{VEC_SEP}2")
    tt |> equal("{f2_w}", "1{VEC_SEP}2")
    if (isRange) {
        tt |> equal("{f2_xx}", "0{VEC_SEP}1")
        tt |> equal("{f2_yy}", "0{VEC_SEP}1")
        tt |> equal("{f2_zz}", "0{VEC_SEP}1")
        tt |> equal("{f2_ww}", "0{VEC_SEP}1")
    } else {
        tt |> equal("{f2_xx}", "1{VEC_SEP}1")
        tt |> equal("{f2_yy}", "1{VEC_SEP}1")
        tt |> equal("{f2_zz}", "1{VEC_SEP}1")
        tt |> equal("{f2_ww}", "1{VEC_SEP}1")
    }
}

def test_vec3(tt : T?; a : auto(vec3) -const -&) {
    let f3_0  = vec3()
    let f3_x  = vec3(1.0, 2.0, 3.0)
    let f3_y  = vec3(1, 2, 3)
    let f3_z  = vec3(0x1, 0x2, 0x3)
    let f3_w  = vec3(1u, 2u, 3u)
    let f3_xx = vec3(1.0)
    let f3_yy = vec3(1)
    let f3_zz = vec3(0x1)
    let f3_ww = vec3(1u)
    tt |> equal("{f3_0}", "0{VEC_SEP}0{VEC_SEP}0")
    tt |> equal("{f3_x}", "1{VEC_SEP}2{VEC_SEP}3")
    tt |> equal("{f3_y}", "1{VEC_SEP}2{VEC_SEP}3")
    tt |> equal("{f3_z}", "1{VEC_SEP}2{VEC_SEP}3")
    tt |> equal("{f3_w}", "1{VEC_SEP}2{VEC_SEP}3")
    tt |> equal("{f3_xx}", "1{VEC_SEP}1{VEC_SEP}1")
    tt |> equal("{f3_yy}", "1{VEC_SEP}1{VEC_SEP}1")
    tt |> equal("{f3_zz}", "1{VEC_SEP}1{VEC_SEP}1")
    tt |> equal("{f3_ww}", "1{VEC_SEP}1{VEC_SEP}1")
}

def test_vec4(tt : T?; a : auto(vec4) -const -&) {
    let f4_0  = vec4()
    let f4_x  = vec4(1.0, 2.0, 3.0, 4.0)
    let f4_y  = vec4(1, 2, 3, 4)
    let f4_z  = vec4(0x1, 0x2, 0x3, 0x4)
    let f4_w  = vec4(1u, 2u, 3u, 4u)
    let f4_xx = vec4(1.0)
    let f4_yy = vec4(1)
    let f4_zz = vec4(0x1)
    let f4_ww = vec4(1u)
    tt |> equal("{f4_0}", "0{VEC_SEP}0{VEC_SEP}0{VEC_SEP}0")
    tt |> equal("{f4_x}", "1{VEC_SEP}2{VEC_SEP}3{VEC_SEP}4")
    tt |> equal("{f4_y}", "1{VEC_SEP}2{VEC_SEP}3{VEC_SEP}4")
    tt |> equal("{f4_z}", "1{VEC_SEP}2{VEC_SEP}3{VEC_SEP}4")
    tt |> equal("{f4_w}", "1{VEC_SEP}2{VEC_SEP}3{VEC_SEP}4")
    tt |> equal("{f4_xx}", "1{VEC_SEP}1{VEC_SEP}1{VEC_SEP}1")
    tt |> equal("{f4_yy}", "1{VEC_SEP}1{VEC_SEP}1{VEC_SEP}1")
    tt |> equal("{f4_zz}", "1{VEC_SEP}1{VEC_SEP}1{VEC_SEP}1")
    tt |> equal("{f4_ww}", "1{VEC_SEP}1{VEC_SEP}1{VEC_SEP}1")
}

[test]
def test_vec_constructors(t : T?) {
    t |> run("float2/3/4 constructors") @(t : T?) {
        test_vec2(t, float2())
        test_vec3(t, float3())
        test_vec4(t, float4())
    }
    t |> run("int2/3/4 constructors") @(t : T?) {
        test_vec2(t, int2())
        test_vec3(t, int3())
        test_vec4(t, int4())
    }
    t |> run("uint2/3/4 constructors") @(t : T?) {
        test_vec2(t, uint2())
        test_vec3(t, uint3())
        test_vec4(t, uint4())
    }
    t |> run("range constructors") @(t : T?) {
        test_vec2(t, range(), true)
        test_vec2(t, urange(), true)
        test_vec2(t, range64(), true)
        test_vec2(t, urange64(), true)
    }
    t |> run("type conversions") @(t : T?) {
        // float <- int
        t |> equal(float2(int2(1, 2)), float2(1, 2))
        t |> equal(float3(int3(1, 2, 3)), float3(1, 2, 3))
        t |> equal(float4(int4(1, 2, 3, 4)), float4(1, 2, 3, 4))
        // float <- uint
        t |> equal(float2(uint2(1u, 2u)), float2(1, 2))
        t |> equal(float3(uint3(1u, 2u, 3u)), float3(1, 2, 3))
        t |> equal(float4(uint4(1u, 2u, 3u, 4u)), float4(1, 2, 3, 4))
        // int <- float
        t |> equal(int2(float2(1, 2)), int2(1, 2))
        t |> equal(int3(float3(1, 2, 3)), int3(1, 2, 3))
        t |> equal(int4(float4(1, 2, 3, 4)), int4(1, 2, 3, 4))
        // uint <- float
        t |> equal(uint2(float2(1, 2)), uint2(1u, 2u))
        t |> equal(uint3(float3(1, 2, 3)), uint3(1u, 2u, 3u))
        t |> equal(uint4(float4(1, 2, 3, 4)), uint4(1u, 2u, 3u, 4u))
    }
}
