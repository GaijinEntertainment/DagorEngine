require daslib/random

var
    seed : int4

typedef
    Foo = tuple<int; float3>

def foo(var attached_fires : array<Foo>)
    let t = random_int(seed) % 100
    for x in attached_fires
        if x._0 == t
            return
    push(attached_fires,[[auto t,float3(0)]])

[export]
def test
    var a : array<Foo>
    for x in range(1000)
        foo(a)
    assert(length(a)==100)
    return true
