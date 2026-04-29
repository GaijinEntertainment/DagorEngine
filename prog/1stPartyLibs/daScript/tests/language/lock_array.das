options gen2

require UnitTest
require dastest/testing_boost public

[test]
def test_lock_array(t : T?) {
    let arr <- [for (x in range(10)); x]
    lock(arr) <| $(tarr) {
        for (a, b in arr, tarr) {
            t |> equal(a, b)
        }
    }
    lock_data(arr) <| $(data, size) {
        for (j in range(10)) {
            unsafe {
                t |> equal(data[j], j)
            }
        }
    }
    var arr2 <- [for (x in range(10)); x]
    lock_data(arr2) <| $(data, size) {
        for (j in range(10)) {
            unsafe {
                data[j] = j * 2
            }
        }
    }
    for (j in range(10)) {
        t |> equal(arr2[j], j * 2)
    }
    lock(arr2) <| $(tarr2) {
        for (a, b in arr2, tarr2) {
            t |> equal(a, b)
        }
    }
    // address of a temp, and pass to implicit
    var farr : array<TestObjectFoo>
    push(farr, TestObjectFoo(uninitialized))
    lock(farr) <| $(ftarr) {
        var q = addr(ftarr[0])
        set_foo_data(q, 1234)
    }
    t |> equal(farr[0].fooData, 1234)
}

