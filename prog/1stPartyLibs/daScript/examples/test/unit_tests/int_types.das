options optimize=false

[sideeffects]
def check12 ( a,b:int; c:auto(TT) )
    let A = TT(a)
    let B = TT(b)
    let A_ = TT(a+1)
    assert(!(A==B))
    assert(A_==B)
    assert(A!=B)
    assert(!(A_!=B))
    assert(c==TT(0))

[export]
def test
    var i = 100500
    var li = 100500100500100500l
    var u = 14u
    var ul = 100500100500100500ul
    var uh = 0x1a
    var uhu = 0x12345678u
    var uhl = 0x123456789abcdefl
    var uhlu = 0x123456789abcdefl
    assert(typeinfo(typename i)=="int")
    assert(typeinfo(typename li)=="int64")
    assert(typeinfo(typename u)=="uint")
    assert(typeinfo(typename ul)=="uint64")
    assert(typeinfo(typename uh)=="uint")
    assert(typeinfo(typename uhu)=="uint")
    assert(typeinfo(typename uhl)=="uint64")
    assert(typeinfo(typename uhlu)=="uint64")
    let ch = 'a'
    assert(ch==97)
    // check small integer operators
    check12(1,2,int8(0))
    check12(1,2,uint8(0))
    check12(1,2,int16(0))
    check12(1,2,uint16(0))
    return true
