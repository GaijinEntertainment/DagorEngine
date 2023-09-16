def addX(a,b)
    assert(b==2 || b==3)
    return a + b

[export]
def test
    let t = 12 |> addX(2) |> addX(3)
    assert(t==17)
    var c = 1 |> addX <| 2   // no () pipe
    c ++
    assert(c==4)
    return true

