// options log = true

[sideeffects]
def onlyNull ( bar : lambda<(a:int):int> )
    if bar != null
        assert(false,"has to be null")

def addX(a:int)
    return @ <| (b:int)
        return a + b

[export]
def test
    var CNT = 0
    let counter <- @ <| (extra:int) : int
        return CNT++ + extra
    var x = invoke(counter,13)
    CNT = 100500
    let y = invoke(counter,13)
    assert(x==13 && y==14)
    let l : lambda<(a:int):int>
    onlyNull(l)
    let t <- addX(1)
    let q = invoke(t,2)
    assert(q==3)
    return true

[export]
def main
    test()

