// options log_infer_passes=true
// options logCpp = true

struct Foo
    i : int = 5
    a : array<int>

var
    g : Foo? = new Foo()

var pg = new Foo()

[export]
def test
    push(g.a, 10)
    assert(g.i==5)
    assert(pg.i==5)
    return true
