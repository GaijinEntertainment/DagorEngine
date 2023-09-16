
require UnitTest

[export]
def test
    let arr <- [{for x in range(10); x}]
    lock(arr) <| $(tarr)
        for a,b in arr,tarr
            assert(a==b)
    lock_data(arr) <| $(data, size)
        for j in range(10)
            unsafe
                assert(data[j]==j)
    var arr2 <- [{for x in range(10); x}]
    lock_data(arr2) <| $(data, size)
        for j in range(10)
            unsafe
                data[j] = j*2
    for j in range(10)
        assert(arr2[j]==j*2)
    lock(arr2) <| $(tarr2)
        for a,b in arr2,tarr2
            assert(a==b)
    // address of a temp, and pass to implicit
    var farr : array<TestObjectFoo>
    push(farr,[[TestObjectFoo]])
    lock(farr) <| $(ftarr)
        var q = addr(ftarr[0])
        set_foo_data(q, 1234)
    assert(farr[0].fooData==1234)
    return true

