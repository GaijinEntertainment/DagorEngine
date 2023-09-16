struct Foo
    value:int

[export]
def test()
    // regular
    var count = 0
    let lam <- @ <| (var a:int &) : bool
        if count < 10
            a = count++
            return true
        else
            return false
    for x,tx in each(lam),range(10)
        assert(x==tx)
    // structure
    let lamF <- @ <| (var a:Foo) : bool
        if count < 10
            a.value = count++
            return true
        else
            return false
    for y,ty in each(lamF),range(10)
        assert(y.value==ty)
    return true
