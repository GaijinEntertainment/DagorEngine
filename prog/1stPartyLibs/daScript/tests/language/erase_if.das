options gen2
require UnitTest
require math
require dastest/testing_boost public

def test_erase_if(t : T?; var arr : array<int>) {
    let cloned := arr
    var maxVal = -1

    for (v in arr) {
        maxVal = max(maxVal, v)
    }

    arr |> erase_if() <| $(v : int) {
        return v < 0
    }

    var ok = true
    for (i, v in count(), arr) {
        if (v < 0) {
            ok = false
        }
    }

    if (length(arr) != maxVal + 1) {
        ok = false
    }

    t |> success(ok)
}

[test]
def test_erase_if_all(t : T?) {
    var t_base : array<int>
    test_erase_if(t, t_base)
    var t0 <- array(0)
    test_erase_if(t, t0)
    var t1 <- array(0, 1)
    test_erase_if(t, t1)
    var t2 <- array(-1)
    test_erase_if(t, t2)
    var t3 <- array(-1, -2)
    test_erase_if(t, t3)
    var t4 <- array(0, -9, 1)
    test_erase_if(t, t4)
    var t5 <- array(-9, -9, 0)
    test_erase_if(t, t5)
    var t6 <- array(-9, 0, -9)
    test_erase_if(t, t6)
    var t7 <- array(0, -9, -9)
    test_erase_if(t, t7)
    var t8 <- array(0, -9, 1, -9)
    test_erase_if(t, t8)
    var t9 <- array(0, -9, 1, 2, -9)
    test_erase_if(t, t9)
    var t10 <- array(0, -9, -5, 1, 2, -9)
    test_erase_if(t, t10)
    var t11 <- array(0, -9, -5, 1, 2, -9, -5)
    test_erase_if(t, t11)
    var t12 <- array(-5, 0, -9, -5, 1, 2, -9, -5)
    test_erase_if(t, t12)
    var t13 <- array(-6, -5, 0, -9, -5, 1, 2, -9, -5)
    test_erase_if(t, t13)
    var t14 <- array(0, -1, 1, -1, -2, 2, 3, 4, -6, -6, 5, 6, 7, -5, -5, -5, -5, 8, 9, -5, -5, -5, -5, -5)
    test_erase_if(t, t14)
    var t15 <- array(0, -1, 1, -1, -2, 2, 3, 4, -6, -6, 5, 6, 7, -5, -5, -5, -5, 8, 9, -5, -5, -5, -5, -5, 10)
    test_erase_if(t, t15)
}
