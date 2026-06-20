options gen2
require dastest/testing_boost public

// Slice A coverage: fused path for arr[i64] / arr[u64] across the
// compute-mode cross-product (Local_Const / Local_Local / Argument).
// Correctness only; fusion-vs-unfused parity is a benchmark concern.

[test]
def test_arr_i64_const_idx(t : T?) {
    var arr : array<int>
    arr |> resize(10)
    for (i in iter_range(arr)) {
        arr[i] = i * 7
    }
    // i64 const literal indices — _I64_Local_Const fusion variant
    t |> equal(arr[0_l], 0)
    t |> equal(arr[3_l], 21)
    t |> equal(arr[9_l], 63)
}

[test]
def test_arr_u64_const_idx(t : T?) {
    var arr : array<int>
    arr |> resize(10)
    for (i in iter_range(arr)) {
        arr[i] = i * 11
    }
    // u64 const literal indices — _U64_Local_Const fusion variant
    t |> equal(arr[0_ul], 0)
    t |> equal(arr[4_ul], 44)
    t |> equal(arr[9_ul], 99)
}

[test]
def test_arr_i64_var_write_read(t : T?) {
    var arr : array<int>
    arr |> resize(5)
    let i : int64 = 2_l
    let j : int64 = 4_l
    arr[i] = 100
    arr[j] = 200
    t |> equal(arr[i], 100)
    t |> equal(arr[j], 200)
}

[test]
def test_arr_i64_via_argument(t : T?) {
    var arr : array<int>
    arr |> resize(8)
    for (i in iter_range(arr)) {
        arr[i] = i + 1000
    }
    t |> equal(read_at(arr, 0_l), 1000)
    t |> equal(read_at(arr, 5_l), 1005)
    t |> equal(read_at(arr, 7_l), 1007)
}

def read_at(arr : array<int>; idx : int64) : int {
    return arr[idx]
}

[test]
def test_arr_i64_float_value_type(t : T?) {
    var arr : array<float>
    arr |> resize(4)
    arr[0_l] = 1.5f
    arr[1_l] = 2.5f
    arr[2_l] = 3.5f
    arr[3_l] = 4.5f
    t |> equal(arr[0_l], 1.5f)
    t |> equal(arr[3_l], 4.5f)
}

[test]
def test_arr_i64_lvalue_write_via_fusion(t : T?) {
    // Exercises ArrayAt_I64 (DAS_PTR_NODE) — writes through the fused ptr return.
    var arr : array<int>
    arr |> resize(4)
    arr[0_l] = 10
    arr[1_l] = 20
    arr[2_l] = 30
    arr[3_l] = 40
    var total = 0
    for (v in arr) {
        total += v
    }
    t |> equal(total, 100)
}

[test]
def test_arr_u64_lvalue_write_via_fusion(t : T?) {
    var arr : array<float>
    arr |> resize(3)
    arr[0_ul] = 1.0f
    arr[1_ul] = 2.0f
    arr[2_ul] = 3.0f
    t |> equal(arr[0_ul] + arr[1_ul] + arr[2_ul], 6.0f)
}
