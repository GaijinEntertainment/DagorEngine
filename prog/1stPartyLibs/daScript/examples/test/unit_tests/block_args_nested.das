def passthrough(a:int; blk:block<(b:int):void>)
    invoke(blk,a)

def passthroughref(a:int&; blk:block<(b:int&):void>)
    invoke(blk,a)

def passthroughptr(a:int?; blk:block<(b:int?):void>)
    invoke(blk,a)

struct Foo
    v : int

def passthroughFoo(a:Foo; blk:block<(b:Foo):void>)
    invoke(blk,a)

[export]
def test
    passthrough(1) <| $ ( a )
        assert(a==1)
        passthrough(2) <| $ ( b )
            assert(a==1 && b==2)
            passthrough(3) <| $ ( c )
                assert(a==1 && b==2 && c==3)

    var A = 1
    var B = 2
    var C = 3
    passthroughref(A) <| $ ( a )
        assert(a==1)
        passthroughref(B) <| $ ( b )
            assert(a==1 && b==2)
            passthroughref(C) <| $ ( c )
                assert(a==1 && b==2 && c==3)

    unsafe
        passthroughptr(addr(A)) <| $ ( a )
            assert(*a==1)
            passthroughptr(addr(B)) <| $ ( b )
                assert(*a==1 && *b==2)
                passthroughptr(addr(C)) <| $ ( c )
                    assert(*a==1 && *b==2 && *c==3)

    passthroughFoo([[Foo v=1]]) <| $ ( a )
        assert(a.v==1)
        passthroughFoo([[Foo v=2]]) <| $ ( b )
            assert(a.v==1 && b.v==2)
            passthroughFoo([[Foo v=3]]) <| $ ( c )
                assert(a.v==1 && b.v==2 && c.v==3)

    return true
