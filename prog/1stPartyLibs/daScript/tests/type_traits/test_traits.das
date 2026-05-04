options gen2
require dastest/testing_boost public

require daslib/type_traits

struct Foo {
    a : int
    b : string
}

struct Bar : Foo {
    c : float
}

[test]
def test_traits(t : T?) {
    t |> equal(2, typeinfo fields_count(type<Foo>))
    t |> equal(3, typeinfo fields_count(type<Bar>))
}


