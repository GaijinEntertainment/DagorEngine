options gen2
require UnitTest
require dastest/testing_boost public

[sideeffects]
def takeFoo(t : T?; var foo : TestObjectFoo; fooRO : TestObjectFoo) {
    t |> success(!foo.isReadOnly)
    t |> success(fooRO.isReadOnly)
}

[test]
def test_add_property_ext_const(t : T?) {
    var foo : TestObjectFoo
    let fooRO : TestObjectFoo
    t |> success(!foo.isReadOnly)
    t |> success(fooRO.isReadOnly)
    takeFoo(t, foo, fooRO)
}

