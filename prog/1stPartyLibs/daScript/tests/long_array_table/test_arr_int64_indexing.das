options gen2
require dastest/testing_boost public

[test]
def test_arr_i64_index_read(t : T?) {
    var arr : array<int>
    arr |> resize(int64(5))
    for (i in iter_range(arr)) {
        arr[i] = i * 10
    }
    let i64_idx : int64 = 2_l
    t |> equal(arr[i64_idx], 20)
}

[test]
def test_arr_u64_index_read(t : T?) {
    var arr : array<int>
    arr |> resize(int64(5))
    for (i in iter_range(arr)) {
        arr[i] = i * 10
    }
    let u64_idx : uint64 = 3_ul
    t |> equal(arr[u64_idx], 30)
}

[test]
def test_arr_i64_index_write(t : T?) {
    var arr : array<int>
    arr |> resize(int64(3))
    let i64_idx : int64 = 1_l
    arr[i64_idx] = 42
    t |> equal(arr[1], 42)
}

[test]
def test_mixed_width_indexing(t : T?) {
    var arr : array<int>
    arr |> resize(int64(4))
    arr[0] = 10
    arr[1_l] = 20
    arr[2_ul] = 30
    arr[uint(3)] = 40
    t |> equal(arr[int(0)] + arr[int64(1)] + arr[uint64(2)] + arr[uint(3)], 100)
}
