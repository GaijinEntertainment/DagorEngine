options gen2
require dastest/testing_boost public
require daslib/array_boost

[test]
def test_resize_int64(t : T?) {
    var arr : array<int>
    arr |> resize(int64(7))
    t |> equal(length(arr), 7)
    t |> equal(long_length(arr), 7_l)
}

[test]
def test_reserve_int64(t : T?) {
    var arr : array<int>
    arr |> reserve(int64(16))
    t |> success(long_capacity(arr) >= 16_l)
}

[test]
def test_erase_int64(t : T?) {
    var arr : array<int>
    arr |> resize(int64(5))
    for (i in iter_range(arr)) {
        arr[i] = i + 1
    }
    arr |> erase(int64(2))
    t |> equal(length(arr), 4)
    t |> equal(arr[2], 4)
}

[test]
def test_erase_range_int64(t : T?) {
    var arr : array<int>
    arr |> resize(int64(5))
    for (i in iter_range(arr)) {
        arr[i] = i + 1
    }
    arr |> erase(int64(1), int64(2))
    t |> equal(length(arr), 3)
    t |> equal(arr[0], 1)
    t |> equal(arr[1], 4)
}

[test]
def test_table_reserve_int64(t : T?) {
    var tab : table<int; int>
    tab |> reserve(int64(64))
    t |> success(long_capacity(tab) >= 64_l)
}

[test]
def test_push_at_int64(t : T?) {
    var arr : array<int>
    arr |> reserve(4)
    arr |> push(10, 0)
    arr |> push(20, 1)
    arr |> push(30, 0_l)  // int64 'at' -- requires int|int64 overload
    t |> equal(length(arr), 3)
    t |> equal(arr[0], 30)
    t |> equal(arr[1], 10)
    t |> equal(arr[2], 20)
}

[test]
def test_push_clone_at_int64(t : T?) {
    var arr : array<int>
    arr |> reserve(4)
    arr |> push_clone(10, 0)
    arr |> push_clone(20, 1)
    arr |> push_clone(30, 0_l)
    t |> equal(length(arr), 3)
    t |> equal(arr[0], 30)
    t |> equal(arr[1], 10)
    t |> equal(arr[2], 20)
}

[test]
def test_emplace_at_int64(t : T?) {
    var arr : array<int>
    arr |> reserve(4)
    var a = 10
    var b = 20
    var c = 30
    arr |> emplace(a, 0)
    arr |> emplace(b, 1)
    arr |> emplace(c, 0_l)
    t |> equal(length(arr), 3)
    t |> equal(arr[0], 30)
    t |> equal(arr[1], 10)
    t |> equal(arr[2], 20)
}

[test]
def test_resize_and_init_int64(t : T?) {
    var arr : array<int>
    arr |> resize_and_init(int64(5))
    t |> equal(length(arr), 5)
    for (i in iter_range(arr)) {
        t |> equal(arr[i], 0)
    }
}

[test]
def test_resize_and_init_initvalue_int64(t : T?) {
    var arr : array<int>
    arr |> resize_and_init(int64(5), 42)
    t |> equal(length(arr), 5)
    for (i in iter_range(arr)) {
        t |> equal(arr[i], 42)
    }
}

[test]
def test_temp_array_data_lenA_int64(t : T?) {
    // array_boost public temp_array(data, lenA, a) -- int64 lenA overload via int|int64
    var src : array<int>
    src |> resize(4)
    src[0] = 10
    src[1] = 20
    src[2] = 30
    src[3] = 40
    unsafe {
        let view <- temp_array(addr(src[0]), 4_l, type<int>)
        t |> equal(length(view), 4)
        t |> equal(view[0], 10)
        t |> equal(view[3], 40)
    }
}

[test]
def test_array_view_int64(t : T?) {
    // array_boost array_view(offset, length : int|int64) -- int64 path
    var bytes : array<int>
    bytes |> resize(8)
    for (i in iter_range(bytes)) {
        bytes[i] = i * 100
    }
    var observed : array<int>
    array_view(bytes, 2_l, 3_l) $(view) {
        for (x in view) {
            observed |> push(x)
        }
    }
    t |> equal(length(observed), 3)
    t |> equal(observed[0], 200)
    t |> equal(observed[1], 300)
    t |> equal(observed[2], 400)
}
