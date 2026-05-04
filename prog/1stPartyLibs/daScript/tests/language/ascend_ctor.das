options gen2
require dastest/testing_boost public

class Foo {
    a : int
    b : int
    def Foo {
        a = 13
    }
}

[sideeffects]
def test_ascend_ctor(t : T?) {
    var foo = new Foo(b = 42)
    t |> equal(13, foo.a)
    t |> equal(42, foo.b)
}

[test]
def test_a_ctor(t : T?) {
    t |> run("ascend constructor") @(t : T?) {
        test_ascend_ctor(t)
    }
}