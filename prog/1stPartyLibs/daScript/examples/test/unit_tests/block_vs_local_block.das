struct Foo
    a : int

var g = 0

def radd(var ext : int; b : block<(arg : int)>)
    invoke(b, ext)

def testLocalBlockVar()
    let localVar = [[Foo a=34]]
    var localBlock = $ <| (arg : int)
        g = localVar.a + arg
    g = 0
    radd(100) <| $(arg : int)
        g = localVar.a + arg
    assert(g == 134)
    g = 0
    invoke(localBlock, 100)
    assert(g == 134)

[export]
def test
    testLocalBlockVar()
    return true

