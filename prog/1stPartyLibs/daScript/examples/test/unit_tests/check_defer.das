require daslib/defer

def check_def(var x:int&)
    defer() <|
        // 2nd
        assert(x==2)
        x *= 2
    defer() <|
        // 1st
        assert(x==3)
        x --
    x +=2

def check_def_del(var a:array<int>)
    a <- [{for x in range(10); x*x}]
    defer_delete(a)
    return length(a)

[sideeffects]
def check_short_pipe
    var x = 1
    if true
        x ++
        defer <|    // notice no (), new syntax
            x --
    assert(x==1)

[export]
def test
    var x = 1
    check_def(x)
    assert(x==4)
    var a : array<int>
    let len = check_def_del(a)
    assert(len==10)
    assert(length(a)==0)
    check_short_pipe()
    return true

