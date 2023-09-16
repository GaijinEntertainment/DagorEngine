// options log=true,log_infer_passes=false,optimize=false

def fold(a:array<auto(numt)>;var from:numt)
    for x in a
        from += x
    return from

[export]
def test:bool
    var a:array<float>
    push(a,1.0)
    push(a,2.0)
    push(a,3.0)
    let q = fold(a,13.0)
    assert(q==19.0)
    return true
