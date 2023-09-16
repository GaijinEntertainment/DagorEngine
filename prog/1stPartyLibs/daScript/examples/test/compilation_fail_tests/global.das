// options log=true, log_infer_passes=false, optimize=true

expect 30113

struct Foo
    i : int = 5
    a : array<int>

var
    g : Foo = new Foo() // 30113: global variable initialization type mismatch, Foo = Foo?

[export]
def test
    push(g.a, 10)
    assert(g.i==5)
    return true
