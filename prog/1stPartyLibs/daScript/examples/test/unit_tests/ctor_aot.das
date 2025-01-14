struct Foo
    a:int
    b:int = -1

[sideeffects]
def take(t:Foo)
    assert(t.a == 10)
    assert(t.b == -1)

[export]
def test
    take(Foo(a=10))
    return true

