options gen2
// options plot=true,plotFile="../../test.dot"
// options log=true, optimize=false
// options no_aot=true
// options log_nodes=true

require UnitTest
require dastest/testing_boost public

[sideeffects]
def make_test_array(var a : array<int>; extra : int = 0) {
    var i = 0
    resize(a, 10)
    for (x in a) {
        x = i++ + extra
    }
}

[sideeffects]
def verify_test_array(t : T?; a : array<int>; extra : int = 0) {
    t |> equal(length(a), 10)
    var i = 0
    for (x in a) {
        t |> equal(x, extra + i++)
    }
}

[sideeffects]
def pass_array(var arr : array<int>) {
    push(arr, 4)
}

[sideeffects]
def verifyIntArray(t : T?; a : array<int>; var s : int; c, tail : int) {
    for (idx in range(c)) {
        t |> equal(a[idx], s++)
    }
    for (idx in range(c, c + tail)) {
        t |> equal(a[idx], 0)
    }
}

[sideeffects]
def testIntArray(t : T?) {
    var arr, arr2 : array<int>
    let val2 = 2
    t |> equal(length(arr), 0)
    push(arr, 1)
    push(arr, val2)
    push(arr, 3)
    t |> equal(length(arr), 3)
    t |> equal(capacity(arr), 16)
    verifyIntArray(t, arr, 1, 3, 0)
    pass_array(arr)
    t |> equal(length(arr), 4)
    verifyIntArray(t, arr, 1, 4, 0)
    resize(arr, 6)
    t |> equal(length(arr), 6)
    verifyIntArray(t, arr, 1, 4, 2)
    resize(arr, 17)
    verifyIntArray(t, arr, 1, 4, 13)
    t |> equal(capacity(arr), 32)
    reserve(arr, 33)
    verifyIntArray(t, arr, 1, 4, 13)
    t |> equal(capacity(arr), 33)
    resize(arr, 4)
    t |> equal(capacity(arr), 33)
    t |> equal(length(arr), 4)
    arr2 <- arr
    t |> equal(length(arr), 0)
    t |> equal(length(arr2), 4)
    var index = 0
    while (index < 4) {
        arr2[index++]++
    }
    verifyIntArray(t, arr2, 2, 4, 0)
    arr <- arr2
    verifyIntArray(t, arr, 2, 4, 0)
    resize(arr, 3)
    verifyIntArray(t, arr, 2, 3, 0)
    resize(arr, 4)
    t |> equal(arr[3], 0)
    verifyIntArray(t, arr, 2, 3, 1)
    push(arr, 1, 0)
    verifyIntArray(t, arr, 1, 4, 1)
    resize(arr, 4)
    push(arr, 5, 4)
    verifyIntArray(t, arr, 1, 5, 0)
    push(arr, 7, 2)
    erase(arr, 2)
    verifyIntArray(t, arr, 1, 5, 0)
    erase(arr, 0)
    erase(arr, 3)
    verifyIntArray(t, arr, 2, 3, 0)
}

def testInterop(t : T?) {
    let res = temp_array_example([ "one" ]) <| $(arr) {
        t |> equal(length(arr), 1)
        t |> equal(arr[0], "one")
    }
    t |> success(res)
}

[test]
def test_array(t : T?) {
    {
        var a : array<int>
        make_test_array(a)
        verify_test_array(t, a)
        make_test_array(a, 13)
        verify_test_array(t, a, 13)
        make_test_array(a)
        verify_test_array(t, a)
        {
            var b : array<int> <- a
            t |> equal(length(a), 0)
            verify_test_array(t, b)
            a <- b
        }
        verify_test_array(t, a)
    }
    {
        var a, b : array<int>
        var i = 0
        resize(a, 10)
        resize(b, 10)
        for (l, r in a, b) {
            l = i
            r = i * 2 - 5
            i++
        }
        verify_test_array(t, a)
        i = 0
        for (l, r in a, b) {
            t |> equal(l, i)
            t |> equal(r, i * 2 - 5)
            i++
        }
        i = 0
        for (l, r in a, b) {
            if (l > r) {
                t |> success(l > r)
                i++
            }
        }
        t |> equal(i, 5)
    }
    {
        var a : array<int>[10]
        var i = 0
        for (b in a) {
            make_test_array(b, i++)
        }
        i = 0
        for (b in a) {
            verify_test_array(t, b, i++)
        }
    }
    testIntArray(t)
    testConstInArray()
    testInterop(t)
    testMultiPush(t)
}

// this test bellow is only here to make sure AOT compiles and runs for array<blah const>

struct Foo {
    bar : int
}

[sideeffects]
def testAccept(t : array<Foo const>; e : Foo const[10]) {
    for (i in t) {
        if (i.bar == 13) {
            return true
        }
    }
    for (j in e) {
        if (j.bar == 13) {
            return false
        }
    }
    return false
}

[sideeffects]
def testConstInArray {
    var a : array<Foo const>
    resize(a, 20)
    var b : Foo const[10]
    testAccept(a, b)
}

[sideeffects]
def testMultiPush(t : T?) {
    var a : array<Foo>
    var b : array<Foo>
    push(b, a)
    t |> equal(length(a), 0)
    t |> equal(length(b), 0)

    a <- [Foo(bar=1), Foo(bar=2)]
    push(b, a)
    t |> equal(length(a), 2)
    t |> equal(length(b), 2)
    t |> equal(a[0].bar, 1)
    t |> equal(a[1].bar, 2)
    t |> equal(b[0].bar, 1)
    t |> equal(b[1].bar, 2)

    var c = fixed_array(Foo(bar = 3))
    push(b, c)
    t |> equal(length(b), 3)
    t |> equal(b[2].bar, 3)
    t |> equal(c[0].bar, 3)

    push_clone(b, c)
    t |> equal(length(b), 4)
    t |> equal(b[3].bar, 3)
}
