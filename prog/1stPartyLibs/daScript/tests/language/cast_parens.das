options gen2
options indenting = 4

require dastest/testing_boost public

[test]
def test_cast_binds_tight(t : T?) {
    // the parenthesized cast is self-delimiting: arithmetic after it applies to the CAST RESULT.
    // (under the old juxtaposition grammar `reinterpret<uint8?>(p) + 3` parsed the `+ 3` INTO the
    // operand — `reinterpret<uint8?>((p) + 3)` — doing void? arithmetic at stride 0, i.e. nothing.)
    var buf : array<uint8>
    buf |> resize(4)
    unsafe {
        var p = reinterpret<uint8?>(addr(buf[0])) + 2
        *p = uint8(0xAB)
        var praw = reinterpret<void?>(addr(buf[0]))
        var q = reinterpret<uint8?>(praw) + 3
        *q = uint8(0xCD)
    }
    t |> equal(uint(buf[0]), 0x0u)
    t |> equal(uint(buf[2]), 0xABu)
    t |> equal(uint(buf[3]), 0xCDu)
}

[test]
def test_cast_postfix_applies_outside(t : T?) {
    // postfix on the cast result — under the old grammar `[1]` would have extended the OPERAND
    var v = float3(1., 2., 3.)
    unsafe {
        let py = reinterpret<float?>(addr(v))[1]   // (cast addr(v)) then index through the pointer
        t |> equal(py, 2.0)
    }
}
