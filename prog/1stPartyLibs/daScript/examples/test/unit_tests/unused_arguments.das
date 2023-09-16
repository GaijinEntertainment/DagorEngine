[sideeffects,unused_argument(b,c)]
def haha(a,b,c)
    return a * 2

[export]
def test
    let t = haha(1,2.0,"blah")
    assert(t==2)
    return true

