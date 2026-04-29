options gen2
require daslib/templates
require dastest/testing_boost public

[template(a), sideeffects]
def boo(x : int; a : auto(TT)) {
    return "{x}_{typeinfo typename(type<TT>)}"
}

[test]
def test_template(t : T?) {
    var a = boo(123, type<float>)
    t |> equal(a, "123_float const")
    var b = boo(456, decltype(2 + 2))
    t |> equal(b, "456_int const")
}
