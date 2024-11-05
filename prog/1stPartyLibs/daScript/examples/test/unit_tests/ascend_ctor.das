class Foo
    a : int
    b : int
    def Foo
        a = 13

[sideeffects]
def test_ascend_ctor
    var foo = new [[Foo() b=42]]
    assert(13 == foo.a)
    assert(42 == foo.b)

[export]
def test
    test_ascend_ctor()
    return true
