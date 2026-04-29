options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

// Tests vector arithmetic operations (*=, /=) with scalar for
// float/int/uint 2/3/4 vectors.

def test_vec2_ops(tt : T?; a : auto(vec2) -const -&) {
    let x : auto(numT) = a[0]
    var mut_a_mul = a
    var mut_a_div = a
    let c = numT(2)
    mut_a_mul *= c
    mut_a_div /= c
    tt |> equal(mut_a_mul, vec2(a[0] * c, a[1] * c))
    tt |> equal(mut_a_div, vec2(a[0] / c, a[1] / c))
}

def test_vec3_ops(tt : T?; a : auto(vec3) -const -&) {
    let x : auto(numT) = a[0]
    var mut_a_mul = a
    var mut_a_div = a
    let c = numT(2)
    mut_a_mul *= c
    mut_a_div /= c
    tt |> equal(mut_a_mul, vec3(a[0] * c, a[1] * c, a[2] * c))
    tt |> equal(mut_a_div, vec3(a[0] / c, a[1] / c, a[2] / c))
}

def test_vec4_ops(tt : T?; a : auto(vec4) -const -&) {
    let x : auto(numT) = a[0]
    var mut_a_mul = a
    var mut_a_div = a
    let c = numT(2)
    mut_a_mul *= c
    mut_a_div /= c
    tt |> equal(mut_a_mul, vec4(a[0] * c, a[1] * c, a[2] * c, a[3] * c))
    tt |> equal(mut_a_div, vec4(a[0] / c, a[1] / c, a[2] / c, a[3] / c))
}

[test]
def test_vec_ops(t : T?) {
    t |> run("float2/3/4 mul-div") @(t : T?) {
        test_vec2_ops(t, float2(3.15, 2.14))
        test_vec3_ops(t, float3(3.15, 2.14, 9.32))
        test_vec4_ops(t, float4(3.15, 2.14, 9.32, 1.23))
    }
    t |> run("int2/3/4 mul-div") @(t : T?) {
        test_vec2_ops(t, int2(6, -5))
        test_vec3_ops(t, int3(6, -5, 8))
        test_vec4_ops(t, int4(6, -9, 13, -12))
    }
    t |> run("uint2/3/4 mul-div") @(t : T?) {
        test_vec2_ops(t, uint2(15, 20))
        test_vec3_ops(t, uint3(15, 20, 25))
        test_vec4_ops(t, uint4(15, 20, 25, 30))
    }
}
