// options plot=true,plotFile="../../test.dot"

[export]
def test:bool
    var a = 1
    let b:int?
    try
        a = deref(b)
    recover
        a = 13
    assert(a==13)
    try
        panic("now panic")
    recover
        a = 14
    assert(a==14)
    return true
