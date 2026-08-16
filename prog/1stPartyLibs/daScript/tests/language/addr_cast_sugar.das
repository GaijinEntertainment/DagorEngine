options gen2
options indenting = 4

require dastest/testing_boost public

[test]
def test_addr_cast_basic(t : T?) {
    // addr<T?>(x) is sugar for reinterpret<T?>(addr(x))
    var v = float3(1., 2., 3.)
    unsafe {
        t |> equal(addr<float?>(v)[1], 2.0)
    }
}

[test]
def test_addr_cast_single_unsafe(t : T?) {
    // one expression-form unsafe() covers BOTH the addr and the reinterpret
    var buf : array<uint8>
    buf |> resize(4)
    var p = unsafe(addr<uint8?>(buf[0]))
    unsafe {
        *(p + 2) = uint8(0xAB)
    }
    t |> equal(uint(buf[2]), 0xABu)
}

[test]
def test_addr_cast_exact_type(t : T?) {
    // exact-type addr<T?> folds to plain addr(x); unsafe still required (see cant_addr_cast_unsafe)
    var i = 42
    var pi = unsafe(addr<int?>(i))
    unsafe {
        *pi = 43
    }
    t |> equal(i, 43)
}

[test]
def test_addr_cast_self_delimiting(t : T?) {
    // postfix applies to the cast result, not the operand
    var v = float3(1., 2., 3.)
    unsafe {
        let q = addr<float?>(v)[2]
        t |> equal(q, 3.0)
    }
}
