// options log=true, log_infer_passes=false, optimize=true

struct Foo
    i : int = 5
    f : float = 6.0
    s : string = "hello"
    n : int64

struct Bar
    i : int

def Bar
    var b : Bar
    b.i = 13
    return b

[export]
def test
    let t = Foo()
    assert(t.i==5)
    let b = Bar()
    assert(b.i==13)
    return true
