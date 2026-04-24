options gen2
options no_aot // `test_abi_func` etc is not declared in headers.

require UnitTest
require dastest/testing_boost public

def jit_test_abi_mad(a, b, c) {
    return test_abi_mad(a, b, c)
}

def test_vec_abi(t : T?) {
    {
        t |> success(!jit_enabled() || is_jit_function(@@ < (a, b, c : float2) : float2 > jit_test_abi_mad))
        let a = float2(1, 2)
        let b = float2(3, 4)
        let c = float2(5, 6)
        let tt = test_abi_mad(a, b, c)
        let jt = jit_test_abi_mad(a, b, c)
        t |> equal(tt, jt)
    }

    {
        t |> success(!jit_enabled() || is_jit_function(@@ < (a, b, c : float3) : float3 > jit_test_abi_mad))
        let a = float3(1, 2, 3)
        let b = float3(4, 5, 6)
        let c = float3(7, 8, 9)
        let tt = test_abi_mad(a, b, c)
        let jt = jit_test_abi_mad(a, b, c)
        t |> equal(tt, jt)
    }

    {
        t |> success(!jit_enabled() || is_jit_function(@@ < (a, b, c : float4) : float4 > jit_test_abi_mad))
        let a = float4(1, 2, 3, 4)
        let b = float4(5, 6, 7, 8)
        let c = float4(9, 10, 11, 12)
        let tt = test_abi_mad(a, b, c)
        let jt = jit_test_abi_mad(a, b, c)
        t |> equal(tt, jt)
    }
}

[sideeffects]
def dummy {
    assert(false)
}

def test_func_abi(t : T?) {
    let fnA = @@dummy
    let fnB = test_abi_func(fnA)
    t |> equal(fnA, fnB)
}

[test]
def test_jit_abi(t : T?) {
    test_vec_abi(t)
    test_func_abi(t)
}
