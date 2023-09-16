[persistent]
class Foo
    count : int = 0
    def Foo ( ic : int )
        count = ic
    def update
        return count++

[export]
def test
    var pFoo = new Foo(13)
    let up1 = pFoo->update()
    let up2 = pFoo->update()
    unsafe
        delete pFoo
    assert(up1==13)
    assert(up2==14)
    return true

