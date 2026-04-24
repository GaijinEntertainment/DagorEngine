options gen2
require dastest/testing_boost public

// Tests dynamic array operations: push, resize, capacity, reserve,
// erase, insert, move (<-), and element initialization.

[test]
def test_dynamic_array(t : T?) {
    t |> run("push and length") @(t : T?) {
        var arr : array<int>
        let val2 = 2
        t |> equal(length(arr), 0)
        push(arr, 1)
        push(arr, val2)
        push(arr, 3)
        t |> equal(length(arr), 3)
        t |> equal(capacity(arr), 16)
    }
    t |> run("pass array by ref") @(t : T?) {
        var arr : array<int>
        push(arr, 1)
        push(arr, 2)
        push(arr, 3)
        pass_array_helper(arr)
        t |> equal(length(arr), 4)
        t |> equal(arr[3], 4)
    }
    t |> run("resize and reserve") @(t : T?) {
        var arr : array<int>
        push(arr, 1)
        push(arr, 2)
        push(arr, 3)
        push(arr, 4)
        resize(arr, 6)
        t |> equal(length(arr), 6)
        resize(arr, 17)
        t |> equal(capacity(arr), 32)
        reserve(arr, 33)
        t |> equal(capacity(arr), 33)
        resize(arr, 4)
        t |> equal(capacity(arr), 33)
        t |> equal(length(arr), 4)
    }
    t |> run("move semantics") @(t : T?) {
        var arr : array<int>
        push(arr, 1)
        push(arr, 2)
        push(arr, 3)
        push(arr, 4)
        var arr2 : array<int>
        arr2 <- arr
        t |> equal(length(arr), 0)
        t |> equal(length(arr2), 4)
        var index = 0
        while (index < 4) {
            arr2[index++]++
        }
        arr <- arr2
        t |> equal(length(arr2), 0)
        t |> equal(length(arr), 4)
        // let-move
        var a : array<int>
        push(a, 1)
        push(a, 2)
        let b <- a
        t |> equal(length(a), 0)
        t |> equal(length(b), 2)
    }
    t |> run("resize zeroes new elements") @(t : T?) {
        var arr : array<int>
        push(arr, 1)
        push(arr, 2)
        push(arr, 3)
        push(arr, 4)
        resize(arr, 3)
        t |> equal(arr[2], 3)
        resize(arr, 4)
        t |> equal(arr[3], 0)
    }
    t |> run("insert and erase") @(t : T?) {
        var arr : array<int>
        push(arr, 2)
        push(arr, 3)
        push(arr, 4)
        // insert at front
        push(arr, 1, 0)
        for (i in range(1, 5)) {
            t |> equal(arr[i - 1], i)
        }
        resize(arr, 4)
        // insert at end
        push(arr, 5, 4)
        // insert in middle, then erase
        push(arr, 7, 2)
        erase(arr, 2)
        for (i in range(1, 5)) {
            t |> equal(arr[i - 1], i)
        }
        // erase first and last
        erase(arr, 0)
        erase(arr, 3)
        for (i in range(2, 4)) {
            t |> equal(arr[i - 2], i)
        }
    }
}

def pass_array_helper(var arr : array<int>) {
    push(arr, 4)
}
