options gen2
require dastest/testing_boost

def vec_at_r2v(vec; idx : int) {
    return vec[idx]
}

def check_vec2_at_r2v(t : T?; v : auto(vecT); a1, a2 : auto(argT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (vec : vecT; idx : int) : argT > vec_at_r2v))
    var f2 = vecT(a1, a2)
    let i1 = vec_at_r2v(f2, 0)
    let i2 = vec_at_r2v(f2, 1)
    t |> equal(a1, i1)
    t |> equal(a2, i2)
}

def check_vec3_at_r2v(t : T?; v : auto(vecT); a1, a2, a3 : auto(argT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (vec : vecT; idx : int) : argT > vec_at_r2v))
    var f2 = vecT(a1, a2, a3)
    let i1 = vec_at_r2v(f2, 0)
    let i2 = vec_at_r2v(f2, 1)
    let i3 = vec_at_r2v(f2, 2)
    t |> equal(a1, i1)
    t |> equal(a2, i2)
    t |> equal(a3, i3)
}

def check_vec4_at_r2v(t : T?; v : auto(vecT); a1, a2, a3, a4 : auto(argT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (vec : vecT; idx : int) : argT > vec_at_r2v))
    var f2 = vecT(a1, a2, a3, a4)
    let i1 = vec_at_r2v(f2, 0)
    let i2 = vec_at_r2v(f2, 1)
    let i3 = vec_at_r2v(f2, 2)
    let i4 = vec_at_r2v(f2, 3)
    t |> equal(a1, i1)
    t |> equal(a2, i2)
    t |> equal(a3, i3)
    t |> equal(a4, i4)
}

def set_vec(var vec &; idx : int; val : auto(argT)) {
    vec[idx] = val
}

def check_set_vec2(t : T?; var v : auto(vecT); a1, a2 : auto(argT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (var vec : vecT&; idx : int; val : argT) > set_vec))
    var f2 : vecT
    set_vec(f2, 0, a1)
    set_vec(f2, 1, a2)
    t |> equal(a1, f2[0])
    t |> equal(a2, f2[1])
}

def check_set_vec3(t : T?; var v : auto(vecT); a1, a2, a3 : auto(argT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (var vec : vecT&; idx : int; val : argT) > set_vec))
    var f2 : vecT
    set_vec(f2, 0, a1)
    set_vec(f2, 1, a2)
    set_vec(f2, 2, a3)
    t |> equal(a1, f2[0])
    t |> equal(a2, f2[1])
    t |> equal(a3, f2[2])
}

def check_set_vec4(t : T?; var v : auto(vecT); a1, a2, a3, a4 : auto(argT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (var vec : vecT&; idx : int; val : argT) > set_vec))
    var f2 : vecT
    set_vec(f2, 0, a1)
    set_vec(f2, 1, a2)
    set_vec(f2, 2, a3)
    set_vec(f2, 3, a4)
    t |> equal(a1, f2[0])
    t |> equal(a2, f2[1])
    t |> equal(a3, f2[2])
    t |> equal(a4, f2[3])
}

[test]
def test_vector_swizzle(t : T?) {
    t |> run("vector2 [] r2v") @(t : T?) {
        check_vec2_at_r2v(t, type<float2>, 1.0, 2.0)
        check_vec2_at_r2v(t, type<int2>, 1, 2)
        check_vec2_at_r2v(t, type<uint2>, 1u, 2u)
        check_vec2_at_r2v(t, type<range>, 1, 2)
        check_vec2_at_r2v(t, type<urange>, 1u, 2u)
        check_vec2_at_r2v(t, type<range64>, 1l, 2l)
        check_vec2_at_r2v(t, type<urange64>, 1ul, 2ul)
    }
    t |> run("vector3 [] r2v") @(t : T?) {
        check_vec3_at_r2v(t, type<float3>, 1.0, 2.0, 3.0)
        check_vec3_at_r2v(t, type<int3>, 1, 2, 3)
        check_vec3_at_r2v(t, type<uint3>, 1u, 2u, 3u)
    }
    t |> run("vector4 [] r2v") @(t : T?) {
        check_vec4_at_r2v(t, type<float4>, 1.0, 2.0, 3.0, 4.0)
        check_vec4_at_r2v(t, type<int4>, 1, 2, 3, 4)
        check_vec4_at_r2v(t, type<uint4>, 1u, 2u, 3u, 4u)
    }
    t |> run("set vector2 []") @(t : T?) {
        check_set_vec2(t, type<float2>, 1.0, 2.0)
        check_set_vec2(t, type<int2>, 1, 2)
        check_set_vec2(t, type<uint2>, 1u, 2u)
        check_set_vec2(t, type<range>, 1, 2)
        check_set_vec2(t, type<urange>, 1u, 2u)
        check_set_vec2(t, type<range64>, 1l, 2l)
        check_set_vec2(t, type<urange64>, 1ul, 2ul)
    }
    t |> run("set vector3 []") @(t : T?) {
        check_set_vec3(t, type<float3>, 1.0, 2.0, 3.0)
        check_set_vec3(t, type<int3>, 1, 2, 3)
        check_set_vec3(t, type<uint3>, 1u, 2u, 3u)
    }
    t |> run("set vector4 []") @(t : T?) {
        check_set_vec4(t, type<float4>, 1.0, 2.0, 3.0, 4.0)
        check_set_vec4(t, type<int4>, 1, 2, 3, 4)
        check_set_vec4(t, type<uint4>, 1u, 2u, 3u, 4u)
    }
}



