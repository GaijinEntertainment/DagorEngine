require UnitTest
require math

def test_erase_if(var arr: array<int>)
    let cloned := arr
    var maxVal = -1

    for v in arr
        maxVal = max(maxVal, v)

    arr |> erase_if() <| $(v: int)
        return v < 0

    var ok = true
    for i, v in count(), arr
        if v < 0
            ok = false

    if length(arr) != maxVal + 1
        ok = false

    if !ok
        print("erase_if failed, input and output:\n")
        debug(cloned)
        debug(arr)
        assert(false)

[export]
def test
    var t: array<int>
    test_erase_if(t)
    var t0 <- array(0)
    test_erase_if(t0)
    var t1 <- array(0, 1)
    test_erase_if(t1)
    var t2 <- array(-1)
    test_erase_if(t2)
    var t3 <- array(-1, -2)
    test_erase_if(t3)
    var t4 <- array(0, -9, 1)
    test_erase_if(t4)
    var t5 <- array(-9, -9, 0)
    test_erase_if(t5)
    var t6 <- array(-9, 0, -9)
    test_erase_if(t6)
    var t7 <- array(0, -9, -9)
    test_erase_if(t7)
    var t8 <- array(0, -9, 1, -9)
    test_erase_if(t8)
    var t9 <- array(0, -9, 1, 2, -9)
    test_erase_if(t9)
    var t10 <- array(0, -9, -5, 1, 2, -9)
    test_erase_if(t10)
    var t11 <- array(0, -9, -5, 1, 2, -9, -5)
    test_erase_if(t11)
    var t12 <- array(-5, 0, -9, -5, 1, 2, -9, -5)
    test_erase_if(t12)
    var t13 <- array(-6, -5, 0, -9, -5, 1, 2, -9, -5)
    test_erase_if(t13)
    var t14 <- array(0, -1, 1, -1, -2, 2, 3, 4, -6, -6, 5, 6, 7, -5, -5, -5, -5, 8, 9, -5, -5, -5, -5, -5)
    test_erase_if(t14)
    var t15 <- array(0, -1, 1, -1, -2, 2, 3, 4, -6, -6, 5, 6, 7, -5, -5, -5, -5, 8, 9, -5, -5, -5, -5, -5, 10)
    test_erase_if(t15)

    return true
