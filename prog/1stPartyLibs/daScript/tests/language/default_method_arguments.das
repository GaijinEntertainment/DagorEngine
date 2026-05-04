options gen2
options strict_smart_pointers

require dastest/testing_boost public

var GOO = 1

class Bar {
    def abstract foobar(a : int = 0) : void
}

class Foo : Bar {
    def override foobar(a : int = 0) : void {
        GOO = a
    }
}

[test]
def test_default_args(t : T?) {
    t |> equal(GOO, 1)
    var a = new Foo()
    a->foobar()
    t |> equal(GOO, 0)
    a->foobar(2)
    t |> equal(GOO, 2)
    (*a)->foobar()
    t |> equal(GOO, 0)
    (*a)->foobar(2)
    t |> equal(GOO, 2)
}
