options gen2
require dastest/testing_boost public

[no_default_initializer]
struct Foo {
    a : int = 5
    b : float = 6.
}

[test]
def test_no_default_initialize(t : T?) {
    var f = Foo(a = 10)
    t |> equal(f.a, 10)
    t |> equal(f.b, 6.)
    f = Foo()
    t |> equal(f.a, 5)
    t |> equal(f.b, 6.)
}
