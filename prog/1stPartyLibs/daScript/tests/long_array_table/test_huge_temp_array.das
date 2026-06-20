options gen2
options persistent_heap = true   // >2 GB arrays need PersistentHeapAllocator

require dastest/testing_boost public
require daslib/fio
require daslib/array_boost

// Memory-gated probe: exercise array_boost.temp_array(arr) on a > INT_MAX
// element source. PR-G widens the private `array_helper` (the path that
// `temp_array(arr)` delegates to) to use long_length + _builtin_make_temp_array_i64
// so the temp view sees the full int64 length. Pre-PR-G the helper used
// `_::length(arr)` (int) and panicked when arr.size > INT_MAX.
//
// Also covers temp_array(data, lenA, a) on an int64 lenA -- this is the
// surface that callers reach when they have a raw pointer and a big length.
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
def test_temp_array_arr_huge(t : T?) {
    if (!huge_enabled()) return
    var arr : array<uint8>
    arr |> resize(HUGE_N)
    arr[0_l] = uint8(0xC0)
    arr[HUGE_N - 1_l] = uint8(0xDE)
    unsafe {
        let view <- temp_array(arr)
        t |> equal(long_length(view), HUGE_N)
        t |> equal(view[0_l], uint8(0xC0))
        t |> equal(view[HUGE_N - 1_l], uint8(0xDE))
    }
    delete arr
}

[test]
def test_temp_array_data_lenA_huge(t : T?) {
    if (!huge_enabled()) return
    var arr : array<uint8>
    arr |> resize(HUGE_N)
    arr[0_l] = uint8(0x11)
    arr[HUGE_N - 1_l] = uint8(0x22)
    unsafe {
        let view <- temp_array(addr(arr[0_l]), HUGE_N, type<uint8>)
        t |> equal(long_length(view), HUGE_N)
        t |> equal(view[0_l], uint8(0x11))
        t |> equal(view[HUGE_N - 1_l], uint8(0x22))
    }
    delete arr
}
