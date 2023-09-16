require UnitTest

[sideeffects]
def takeFoo(var foo:TestObjectFoo; fooRO:TestObjectFoo)
    verify(!foo.isReadOnly)
    assert(fooRO.isReadOnly)

[export]
def test
    var foo: TestObjectFoo
    let fooRO : TestObjectFoo
    verify(!foo.isReadOnly)
    assert(fooRO.isReadOnly)
    takeFoo(foo,fooRO)
    return true

