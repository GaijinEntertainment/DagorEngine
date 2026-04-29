options gen2
require dastest/testing_boost public

struct Foo {
    a : int
}

struct Bar {
    foo : Foo
}

struct Car {
    bar : Bar
}

[test]
def test_assume(t : T?) {
    var car : Car
    car.bar.foo.a = 1
    assume cbf = car.bar.foo
    t |> equal(cbf.a, 1)
    assume cbfa = cbf.a
    t |> equal(cbfa, 1)
    cbfa ++
    t |> equal(car.bar.foo.a, 2)
}
