options gen2
require dastest/testing_boost public

// Tests dynamic array operations: push, resize, capacity, reserve,
// erase, insert, move (<-), and element initialization.

[test]
def test_dynamic_array(t : T?) { // nolint:STYLE038 — flat list of independent container-op arms
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
    t |> run("resize rounds capacity to the next power of two") @(t : T?) {
        var min : array<int>
        resize(min, 3)                  // below the floor -> min capacity 16
        t |> equal(capacity(min), 16)
        var tiny : array<int>
        resize(tiny, 63)                // rounds UP to the next pow2
        t |> equal(capacity(tiny), 64)
        var big : array<int>
        resize(big, 65)                 // pow2 slack keeps append-by-resize amortized O(1)
        t |> equal(capacity(big), 128)
        var huge : array<int>
        resize(huge, 1000)
        t |> equal(capacity(huge), 1024)
        var exact : array<int>
        reserve(exact, 1000)            // reserve is exact: N set, N get
        resize(exact, 1000)             // a reserved resize never grows, so no pow2 round-up
        t |> equal(capacity(exact), 1000)
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
