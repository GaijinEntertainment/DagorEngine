require fio

[safe_when_uninitialized]
struct Foo
    a:int = 1
    b:float = 2.0

[export]
def test:bool
    var saved = false
    var loaded = false
    let foo0 = Foo()
    fopen("test.bin","wb") <| $(f)
        if f!=null
            fsave(f,foo0)
            saved = true
    var foo1 : Foo
    fopen("test.bin","rb") <| $(f)
        if f!=null
            fload(f,foo1)
            loaded = true
    assert(saved)
    assert(loaded)
    assert(foo0.a==foo1.a && foo0.b==foo1.b)
    return true

