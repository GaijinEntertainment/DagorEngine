options gen2
require daslib/functional
require dastest/testing_boost public

[test]
def test_long_iter_range(t : T?) {
    var arr : array<int>
    arr |> resize(int64(5))
    for (i in iter_range(arr)) {
        arr[i] = i + 1
    }
    var sum = 0_l
    for (i in long_iter_range(arr)) {
        sum += int64(arr[i])
    }
    t |> equal(sum, 15_l)
}

[test]
def test_long_enumerate(t : T?) {
    var arr : array<int>
    arr |> resize(int64(4))
    arr[0] = 10
    arr[1] = 20
    arr[2] = 30
    arr[3] = 40
    var sum_idx = 0_l
    var sum_val = 0
    unsafe {
        for ((i, v) in long_enumerate(each(arr))) {
            sum_idx += i
            sum_val += v
        }
    }
    t |> equal(sum_idx, 0_l + 1_l + 2_l + 3_l)
    t |> equal(sum_val, 100)
}

[test]
def test_long_find_index_present(t : T?) {
    var arr : array<int>
    arr |> resize(int64(5))
    arr[0] = 100
    arr[1] = 200
    arr[2] = 300
    arr[3] = 400
    arr[4] = 500
    t |> equal(long_find_index(arr, 300), 2_l)
}

[test]
def test_long_find_index_missing(t : T?) {
    var arr : array<int>
    arr |> resize(int64(3))
    arr[0] = 1
    arr[1] = 2
    arr[2] = 3
    t |> equal(long_find_index(arr, 99), -1_l)
}
