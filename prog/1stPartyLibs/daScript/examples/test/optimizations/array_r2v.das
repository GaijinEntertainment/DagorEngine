// options log_nodes = true, log_stack = true

struct Foo
    x,y,z,w:int

def testA(var a)
    for x in range(10)
        a[x].y = x
    a[3].z = 13
    for x in range(10)
        if x!=3
            assert(a[x].x==0 && a[x].y==x && a[x].z==0 && a[x].w==0)
        else
            assert(a[x].x==0 && a[x].y==x && a[x].z==13 && a[x].w==0)
    assert(a[3].z==13)

[export]
def test
    // regular array, struct
    var a : Foo[10]
    testA(a)
    // array<struct>
    var b : array<Foo>
    resize(b,10)
    testA(b)
    // table<int;int>
    var c : table<int;Foo>
    testA(c)
    return true


