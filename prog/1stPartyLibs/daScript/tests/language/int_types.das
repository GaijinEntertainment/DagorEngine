options gen2
options no_unused_function_arguments = false
options optimize = false
require dastest/testing_boost public

[sideeffects]
def it_check12(a, b : int; c : auto(TT)) {
    let A = TT(a)
    let B = TT(b)
    let A_ = TT(a + 1)
    assert(!(A == B))
    assert(A_ == B)
    assert(A != B)
    assert(!(A_ != B))
    assert(c == TT(0))
}

[test]
def test_int_types(t : T?) {
    t |> run("integer literal types") @(t : T?) {
        var i = 100500
        var li = 100500100500100500l
        var u = 14u
        var ul = 100500100500100500ul
        var uh = 0x1a
        var uhu = 0x12345678u
        var uhl = 0x123456789abcdefl
        var uhlu = 0x123456789abcdefl
        t |> equal(typeinfo typename(i), "int")
        t |> equal(typeinfo typename(li), "int64")
        t |> equal(typeinfo typename(u), "uint")
        t |> equal(typeinfo typename(ul), "uint64")
        t |> equal(typeinfo typename(uh), "uint")
        t |> equal(typeinfo typename(uhu), "uint")
        t |> equal(typeinfo typename(uhl), "uint64")
        t |> equal(typeinfo typename(uhlu), "uint64")
    }
    t |> run("char literal") @(t : T?) {
        let ch = 'a'
        t |> equal(ch, 97)
    }
    t |> run("small integer operators") @(t : T?) {
        it_check12(1, 2, int8(0))
        it_check12(1, 2, uint8(0))
        it_check12(1, 2, int16(0))
        it_check12(1, 2, uint16(0))
        t |> success(true)
    }
}
