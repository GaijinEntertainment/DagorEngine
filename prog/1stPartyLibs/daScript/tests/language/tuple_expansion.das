options gen2
require dastest/testing_boost public

def foo {
    return (1, "2", 3.0)
}

[test]
def test_tuple_expansion(t : T?) {
    let (i, s, f) = foo()
    t |> equal(i, 1)
    t |> equal(s, "2")
    t |> equal(f, 3.0)
    t |> equal(typeinfo sizeof(type<tuple<int; string; float>>), typeinfo sizeof(i`s`f))
}
