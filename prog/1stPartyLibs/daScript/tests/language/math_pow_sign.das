options gen2

require dastest/testing_boost public
require math

var g3 : float3

[test]
def test_vector_pow_keeps_sign(t : T?) {
    g3 = float3(-2.0, 2.0, -3.0)
    let odd = pow(g3, float3(3.0))
    t |> equal(odd.x, -8.0, "pow(-2, 3) is -8")
    t |> equal(odd.y, 8.0, "a positive base is untouched")
    t |> equal(pow(g3, float3(4.0)).x, 16.0, "an even exponent leaves the sign alone")
    t |> success(pow(g3, float3(-3.0)).x < 0.0, "a negative odd exponent flips too")
    t |> equal(pow(g3, float3(0.0)).x, 1.0, "pow(x, 0) is 1")
    t |> equal(pow_est(g3, float3(3.0)).x, 8.0, "pow_est drops the sign - it is GLSL pow")
}
