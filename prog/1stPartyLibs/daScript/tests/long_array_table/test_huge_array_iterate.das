options gen2
options persistent_heap = true   // >2 GB arrays need PersistentHeapAllocator

require dastest/testing_boost public
require daslib/fio
require daslib/functional

// Memory-gated probe: iterate a > INT_MAX-element array four different ways
// and assert each visits exactly long_length(arr) elements. If any iteration
// shape uses an int counter internally, the count check fails (or wraps).
//
// Auto-discovered by dastest; inline `huge_enabled()` gate silent-returns when
// the env var is unset, so CI sees a no-op pass. Manual run actually allocates.

def huge_enabled() : bool {
    static_if (typeinfo sizeof(type<int?>) < 8) {
        return false
    }
    return has_env_variable("DASLANG_HUGE_HEAP_TESTS") && get_env_variable("DASLANG_HUGE_HEAP_TESTS") == "1"
}

// 2.2 GB elements of uint8 — exceeds INT_MAX (2.147 GB), stays under
// UINT32_MAX (4 GB) for faster runs.
let HUGE_N = 2200_l * 1024_l * 1024_l

[test]
def test_iterate_via_range64_long_length(t : T?) {
    if (!huge_enabled()) return
    var arr : array<uint8>
    arr |> resize(HUGE_N)
    var count = 0_l
    for (_i in range64(0_l, long_length(arr))) {
        count++
    }
    t |> equal(count, HUGE_N)
    delete arr
}

[test]
def test_iterate_via_long_iter_range(t : T?) {
    if (!huge_enabled()) return
    var arr : array<uint8>
    arr |> resize(HUGE_N)
    var count = 0_l
    for (_i in long_iter_range(arr)) {
        count++
    }
    t |> equal(count, HUGE_N)
    delete arr
}

[test]
def test_iterate_via_index_read(t : T?) {
    if (!huge_enabled()) return
    var arr : array<uint8>
    arr |> resize(HUGE_N)
    // Write a sentinel at the last index, iterate with int64 counter and
    // assert we read it back via arr[i] indexing (stride math).
    arr[HUGE_N - 1_l] = uint8(0x77)
    var sum = 0_l
    for (i in range64(0_l, long_length(arr))) {
        sum += int64(arr[i])
    }
    t |> equal(sum, 119_l)  // only one non-zero byte at the very end (0x77)
    delete arr
}

[test]
def test_iterate_via_long_enumerate(t : T?) {
    if (!huge_enabled()) return
    var arr : array<uint8>
    arr |> resize(HUGE_N)
    var last_index = -1_l
    var count = 0_l
    unsafe {
        for ((i, _v) in long_enumerate(each(arr))) {
            last_index = i
            count++
        }
    }
    t |> equal(count, HUGE_N)
    t |> equal(last_index, HUGE_N - 1_l)
    delete arr
}
