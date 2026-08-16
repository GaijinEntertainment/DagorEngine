options gen2
require dastest/testing_boost public
require math

// hmin / hmax / hadd horizontal reduce for float{2,3,4}. The values are read from heap
// arrays, so the element reads are not const-folded and the calls exercise the real
// interp / AOT / JIT codegen (the JIT lane thus covers the llvm.vector.reduce.* lowering).

[test]
def test_horizontal_reduce(t : T?) {
    let v4 <- [float4(3.0, 1.0, 4.0, 1.5), float4(-2.0, 5.0, -7.0, 0.0)]
    let v3 <- [float3(3.0, 1.0, 4.0)]
    let v2 <- [float2(3.0, 1.0)]

    t |> equal(hmin(v4[0]), 1.0)
    t |> equal(hmax(v4[0]), 4.0)
    t |> equal(hadd(v4[0]), 9.5)
    t |> equal(hmin(v4[1]), -7.0)
    t |> equal(hmax(v4[1]), 5.0)
    t |> equal(hadd(v4[1]), -4.0)

    t |> equal(hmin(v3[0]), 1.0)
    t |> equal(hmax(v3[0]), 4.0)
    t |> equal(hadd(v3[0]), 8.0)

    t |> equal(hmin(v2[0]), 1.0)
    t |> equal(hmax(v2[0]), 3.0)
    t |> equal(hadd(v2[0]), 4.0)
}
