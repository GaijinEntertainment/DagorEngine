struct Foo
    a : int

[sideeffects]
def buggy(skip:bool)
    var a = new Foo
    if skip
        return
    var b = new Foo
finally
    unsafe
        delete a
        delete b

[export]
def test
    buggy(true)
    return true
