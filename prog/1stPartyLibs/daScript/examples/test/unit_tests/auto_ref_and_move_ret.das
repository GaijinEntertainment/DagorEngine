def abs(i:int)
    return i<0 ? -i : i

def foo(var bar:array<int>)
    var b & = unsafe(bar)
    for x in range(10)
        push(b,x)
    return <- b

[export]
def test
    var a : array<int>
    for x in range(10)
        push(a,10-x)
    var t <- foo(a)
    assert(length(a)==0)
    for x,y in range(-10,10),t
        assert(abs(x)==y)
    return true

