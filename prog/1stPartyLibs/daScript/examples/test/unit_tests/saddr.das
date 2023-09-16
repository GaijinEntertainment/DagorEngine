require daslib/safe_addr

struct Foo
    a : int

[export]
def test
    var t = 1
    var pt = safe_addr(t)
    *pt = 2
    assert(t==2)
    let q = [[Foo a=3]]
    let pq = safe_addr(q.a)
    assert(*pq==3)
    var w : int[2]
    w[1] = 4
    let pw = safe_addr(w[1])
    assert(*pw==4)
    return true


