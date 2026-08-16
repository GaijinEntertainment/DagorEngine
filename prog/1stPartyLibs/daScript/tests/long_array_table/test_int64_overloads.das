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

[test]
def test_array_view_borrowed_subview(t : T?) {
    let values <- [10, 20, 30, 40]
    array_view(values, 0, length(values)) $(whole : array<int>#) {
        array_view(whole, 1, 2) $(middle : array<int>#) {
            t |> equal(length(middle), 2)
            t |> equal(middle[0], 20)
            t |> equal(middle[1], 30)
        }
        array_view(whole, length(whole), 0) $(empty : array<int>#) {
            t |> equal(length(empty), 0)
        }
    }
}

[test]
def test_array_view_typed_bytes(t : T?) {
    let bytes <- [
        uint8(1), uint8(0), uint8(0), uint8(0),
        uint8(0xFE), uint8(0xFF), uint8(0xFF), uint8(0xFF)
    ]
    array_view(bytes, 0, 2, type<int>) $(values : array<int>#) {
        t |> equal(length(values), 2)
        t |> equal(values[0], 1)
        t |> equal(values[1], -2)
    }
    array_view(bytes, 0_l, long_length(bytes)) $(borrowed : array<uint8>#) {
        array_view(borrowed, 4_l, 1_l, type<int>) $(tail : array<int>#) {
            t |> equal(length(tail), 1)
            t |> equal(tail[0], -2)
        }
    }
    array_view(bytes, length(bytes), 0, type<int>) $(empty : array<int>#) {
        t |> equal(length(empty), 0)
    }

    var unaligned_panicked = false
    try {
        array_view(bytes, 1, 1, type<int>) $(_ : array<int>#) {
        }
    } recover {
        unaligned_panicked = true
    }
    t |> success(unaligned_panicked, "typed array_view rejects unaligned data")

    var out_of_range_panicked = false
    try {
        array_view(bytes, 4, 2, type<int>) $(_ : array<int>#) {
        }
    } recover {
        out_of_range_panicked = true
    }
    t |> success(out_of_range_panicked, "typed array_view rejects out-of-range data")
}

[test]
def test_pod_view(t : T?) {
    // 0x04030201 at offset 1, 0x08070605 at offset 5, fp32 3.5 at offset 9 — all unaligned
    let bytes <- [
        uint8(0xAA),
        uint8(0x01), uint8(0x02), uint8(0x03), uint8(0x04),
        uint8(0x05), uint8(0x06), uint8(0x07), uint8(0x08),
        uint8(0x00), uint8(0x00), uint8(0x60), uint8(0x40)
    ]
    t |> equal(pod_view(bytes, 1, type<int>), int(0x04030201))
    t |> equal(pod_view(bytes, 5, type<int>), int(0x08070605))
    t |> equal(pod_view(bytes, 9, type<float>), 3.5)
    t |> equal(pod_view(bytes, 1_l, type<int>), int(0x04030201))
    let pair = pod_view(bytes, 1, type<int2>)
    t |> equal(pair.x, int(0x04030201))
    t |> equal(pair.y, int(0x08070605))
    array_view(bytes, 0, length(bytes)) $(borrowed : array<uint8>#) {
        t |> equal(pod_view(borrowed, 5, type<int>), int(0x08070605))
    }

    var sink = 0
    var out_of_range_panicked = false
    try {
        sink = pod_view(bytes, 10, type<int>)
    } recover {
        out_of_range_panicked = true
    }
    t |> success(out_of_range_panicked, "pod_view rejects out-of-range reads")
    t |> equal(sink, 0)

    var negative_panicked = false
    try {
        sink = pod_view(bytes, -1, type<int>)
    } recover {
        negative_panicked = true
    }
    t |> success(negative_panicked, "pod_view rejects negative offsets")
    t |> equal(sink, 0)
}
