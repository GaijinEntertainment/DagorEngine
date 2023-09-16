require daslib/if_not_null

struct Foo
    a : int

[sideeffects]
def take3(var pfoo:Foo; b,c:int)
    assert(pfoo.a==1 && b==2 && c==3)

[export]
def test
    var pfoo : Foo?
    pfoo |> if_not_null() <| take3(0,0)
    pfoo = new [[Foo a=1]]
    pfoo |> if_not_null() <| take3(2,3)
    return true

