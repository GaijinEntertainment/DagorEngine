[export]
def test
    var x = 1.0f
    memzero(x)
    assert(x==0.0f)
    var t = [[int 1;2]]
    memzero(t)
    assert(t[0]==0 && t[1]==0)
    return true
