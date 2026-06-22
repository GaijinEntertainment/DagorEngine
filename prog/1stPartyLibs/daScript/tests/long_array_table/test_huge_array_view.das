options gen2
options persistent_heap = true

require dastest/testing_boost public
require daslib/fio
require daslib/array_boost

// Memory-gated probe: exercise array_boost.array_view with int64 offset/length
// on a > INT_MAX element source. PR-G consolidates the prior int+int64
// overload pair to a single int|int64 signature with static_if dispatch
// (mirrors the PR-builtin pattern). The int64 path uses long_length and
// _builtin_make_temp_array_i64, and the bounds check uses uint64 arithmetic
// to avoid signed overflow.
//
// Skipped silently unless DASLANG_HUGE_HEAP_TESTS=1 on a 64-bit build.

def huge_enabled() : bool {
    static_if (typeinfo sizeof(type<int?>) < 8) {
        return false
    }
    return has_env_variable("DASLANG_HUGE_HEAP_TESTS") && get_env_variable("DASLANG_HUGE_HEAP_TESTS") == "1"
}

// 2.2 GB elements of uint8 -- exceeds INT_MAX (2.147 GB).
let HUGE_N = 2200_l * 1024_l * 1024_l

[test]
def test_array_view_offset_int64_past_int_max(t : T?) {
    if (!huge_enabled()) return
    var arr : array<uint8>
    arr |> resize(HUGE_N)
    // Sentinel near the tail (offset past INT_MAX).
    arr[HUGE_N - 5_l] = uint8(0xAA)
    arr[HUGE_N - 4_l] = uint8(0xBB)
    arr[HUGE_N - 3_l] = uint8(0xCC)
    array_view(arr, HUGE_N - 5_l, 3_l) $(view) {
        t |> equal(length(view), 3)
        t |> equal(view[0], uint8(0xAA))
        t |> equal(view[1], uint8(0xBB))
        t |> equal(view[2], uint8(0xCC))
    }
    delete arr
}

[test]
def test_array_view_var_int64_past_int_max(t : T?) {
    if (!huge_enabled()) return
    var arr : array<uint8>
    arr |> resize(HUGE_N)
    arr[HUGE_N - 2_l] = uint8(0x77)
    arr[HUGE_N - 1_l] = uint8(0x88)
    array_view(arr, HUGE_N - 2_l, 2_l) $(var view) {
        view[0] = uint8(0x99)
        view[1] = uint8(0xAA)
    }
    t |> equal(arr[HUGE_N - 2_l], uint8(0x99))
    t |> equal(arr[HUGE_N - 1_l], uint8(0xAA))
    delete arr
}
