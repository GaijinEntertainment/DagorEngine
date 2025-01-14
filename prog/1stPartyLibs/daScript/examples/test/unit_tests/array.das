// options plot=true,plotFile="../../test.dot"
// options log=true, optimize=false
// options no_aot=true
// options log_nodes=true

require UnitTest

[sideeffects]
def make_test_array(var a:array<int>;extra:int=0)
    var i=0
    resize(a,10)
    for x in a
        x = i++ + extra

[sideeffects]
def verify_test_array(a:array<int>;extra:int=0)
    assert(length(a)==10)
    var i=0
    for x in a
        verify(x==(extra+i++))

[sideeffects]
def pass_array ( var arr:array<int> )
    push(arr,4)

[sideeffects]
def verifyIntArray(a:array<int>;var s:int;c,tail:int)
    for t in range(c)
        verify(a[t]==s++)
    for t in range(c,c+tail)
        assert(a[t]==0)

[sideeffects]
def testIntArray
    var arr,arr2:array<int>
    let val2 = 2
    assert(length(arr)==0,"array starts at 0 length")
    push(arr,1)
    push(arr,val2)
    push(arr,3)
    assert(length(arr)==3,"array length is 3")
    assert(capacity(arr)==16,"its 2x, but no less than 16")
    verifyIntArray(arr,1,3,0)
    pass_array(arr)
    assert(length(arr)==4,"array length is 4")
    verifyIntArray(arr,1,4,0)
    resize(arr,6)
    assert(length(arr)==6,"array length is now 6")
    verifyIntArray(arr,1,4,2)
    resize(arr,17)
    verifyIntArray(arr,1,4,13)
    assert(capacity(arr)==32,"now bigger")
    reserve(arr,33)
    verifyIntArray(arr,1,4,13)
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
    verifyIntArray(arr2,2,4,0)
    arr <- arr2
    verifyIntArray(arr,2,4,0)
    resize(arr,3)
    verifyIntArray(arr,2,3,0)
    resize(arr,4)
    assert(arr[3]==0,"new element after resize is 0")
    verifyIntArray(arr,2,3,1)
    push(arr,1,0)
    verifyIntArray(arr,1,4,1)
    resize(arr,4)
    push(arr,5,4)
    verifyIntArray(arr,1,5,0)
    push(arr,7,2)
    erase(arr,2)
    verifyIntArray(arr,1,5,0)
    erase(arr,0)
    erase(arr,3)
    verifyIntArray(arr,2,3,0)

def testInterop
    let res = temp_array_example([{ string[] "one" }]) <| $ ( arr )
        assert(length(arr)==1)
        assert(arr[0]=="one")
    assert(res)

[export]
def test:bool
    if true
        var a:array<int>
        make_test_array(a)
        verify_test_array(a)
        make_test_array(a,13)
        verify_test_array(a,13)
        make_test_array(a)
        verify_test_array(a)
        if true
            var b:array<int> <- a
            assert(length(a)==0)
            verify_test_array(b)
            a <- b
        verify_test_array(a)
    if true
        var a,b:array<int>
        var i = 0
        resize(a,10)
        resize(b,10)
        for l,r in a,b
            l = i
            r = i*2-5
            i++
        verify_test_array(a)
        i = 0
        for l,r in a,b
            assert(l==i)
            assert(r==i*2-5)
            i++
        i = 0
        for l,r in a,b
            if l>r
                assert(l>r)
                i++
        assert(i==5)
    if true
        var a:array<int>[10]
        var i = 0
        for b in a
            make_test_array(b,i++)
        i = 0
        for b in a
            verify_test_array(b,i++)
    testIntArray()
    testConstInArray()
    testInterop()
    testMultiPush()
    return true

// this test bellow is only here to make sure AOT compiles and runs for array<blah const>

struct Foo
    bar : int

[sideeffects]
def testAccept(t:array<Foo const>;e:Foo const[10])
    for i in t
        if i.bar == 13
            return true
    for j in e
        if j.bar == 13
            return false
    return false

[sideeffects]
def testConstInArray
    var a : array<Foo const>
    resize(a,20)
    var b : Foo const[10]
    testAccept(a, b)

[sideeffects]
def testMultiPush()
    var a : array<Foo>
    var b : array<Foo>
    push(b, a)
    verify(length(a)==0)
    verify(length(b)==0)

    a <- [Foo(bar=1), Foo(bar=2)]
    push(b, a)
    verify(length(a)==2)
    verify(length(b)==2)
    verify(a[0].bar==1)
    verify(a[1].bar==2)
    verify(b[0].bar==1)
    verify(b[1].bar==2)

    var c = fixed_array(Foo(bar=3))
    push(b, c)
    verify(length(b)==3)
    verify(b[2].bar==3)
    verify(c[0].bar==3)

    push_clone(b, c)
    verify(length(b)==4)
    verify(b[3].bar==3)
