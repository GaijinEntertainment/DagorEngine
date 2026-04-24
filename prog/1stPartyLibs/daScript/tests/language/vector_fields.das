options gen2
options persistent_heap = true
options gc

require dastest/testing_boost public


[test]
def test_vector_fields(t : T?) {
    t |> run("basic fields") @(t : T?) {
        var v : float4
        v.r = 1.
        v.g = 2.
        v.b = 3.
        v.a = 4.
        t |> equal(float4(1., 2., 3., 4.), v)
    }
    t |> run("swizzle") @(t : T?) {
        var v : float4
        v.rg = float2(1., 2.)
        v.ba = float2(3., 4.)
        t |> equal(float4(1., 2., 3., 4.), v)
        t |> equal(float4(4., 3., 2., 1.), v.abgr)
    }
}

