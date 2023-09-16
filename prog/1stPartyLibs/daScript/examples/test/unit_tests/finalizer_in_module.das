module haha

var
    all_good = false

struct Foo
    a : int

def operator delete (var f : Foo)
    all_good = true

[export]
def test
    var arr : array<Foo>
    push(arr,[[Foo a=1]])
    delete arr
    assert(all_good)
    return true
