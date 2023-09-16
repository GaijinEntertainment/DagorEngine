def add(a,b:int):int
    return a + b

[export]
def test
    let fn = @@add
    let t = invoke(fn, 1, 2)
    assert(t==3)
    assert(fn!=null)
    assert(fn==@@add)
    assert(!(fn==null))
    assert(!(fn!=@@add))
    let qq : function
    assert(qq==null)
    var failed = false
    try
        let fn2 : function<(a,b:int):int>
        let t2 = invoke(fn2, 1, 2)
    recover
        failed = true
    assert(failed,"has to fail, invoking null function")
    return true

