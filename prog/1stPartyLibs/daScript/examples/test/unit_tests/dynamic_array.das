// options log=true

def pass_array ( var arr )
    push(arr,4)

[export]
def test
    if true
        var arr,arr2:array<int>
        let val2 = 2
        assert(length(arr)==0,"array starts at 0 length")
        push(arr,1)
        push(arr,val2)
        push(arr,3)
        assert(length(arr)==3,"array length is 3")
        assert(capacity(arr)==16,"its 2x, but no less than 16")
        pass_array(arr)
        assert(length(arr)==4,"array length is 4")
        resize(arr,6)
        assert(length(arr)==6,"array length is now 6")
        resize(arr,17)
        assert(capacity(arr)==32,"now bigger")
        reserve(arr,33)
        assert(capacity(arr)==33,"now even bigger")
        resize(arr,4)
        assert(capacity(arr)==33,"did not grow smaller")
        assert(length(arr)==4,"resized properly")
        arr2 <- arr
        assert(length(arr)==0)
        assert(length(arr2)==4)
        var index = 0
        while index < 4
            arr2[index++]++
        arr <- arr2
        assert(length(arr2)==0)
        assert(length(arr)==4)
        resize(arr,3)
        assert(arr[2]==4)
        resize(arr,4)
        assert(arr[3]==0,"new element after resize is 0")
        push(arr,1,0)
        for t in range(1,5)
            assert(arr[t-1]==t)
        resize(arr,4)
        push(arr,5,4)
        push(arr,7,2)
        erase(arr,2)
        for t in range(1,5)
            assert(arr[t-1]==t)
        erase(arr,0)
        erase(arr,3)
        for t in range(2,4)
            assert(arr[t-2]==t)
        // move while initializing
        var a:array<int>
        push(a,1)
        push(a,2)
        let b <- a
        assert(length(a)==0 && length(b)==2)
    return true
