options gen2
options persistent_heap = true   // >2 GB arrays need PersistentHeapAllocator

require dastest/testing_boost public
require daslib/fio

// Memory-gated probe: erase at int64 position / int64 range past INT_MAX.
// The int64 overloads of `erase` (daslib/builtin.das:457, 465) were already
// widened pre-PR-builtin and live-tested with small N via
// test_int64_overloads.das, but their behavior at positions > INT_MAX was
// never exercised. This probe fills that gap by allocating a 2.2 GB uint8
// array and erasing near the tail (cheap memmove, exercises the full int64
// path through __builtin_array_erase_i64 / __builtin_array_erase_range_i64).
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
def test_erase_at_int64_past_int_max(t : T?) {
    if (!huge_enabled()) return
    var arr : array<uint8>
    arr |> resize(HUGE_N)
    let before = long_length(arr)
    let at_pos = before - 2_l
    arr[at_pos] = uint8(0xDD)
    arr[at_pos + 1_l] = uint8(0xEE)
    arr |> erase(at_pos)
    t |> equal(long_length(arr), before - 1_l)
    t |> equal(arr[at_pos], uint8(0xEE))
    delete arr
}

[test]
def test_erase_range_at_int64_past_int_max(t : T?) {
    if (!huge_enabled()) return
    var arr : array<uint8>
    arr |> resize(HUGE_N)
    let before = long_length(arr)
    let at_pos = before - 5_l
    arr[at_pos + 4_l] = uint8(0xFF)
    arr |> erase(at_pos, 3_l)
    t |> equal(long_length(arr), before - 3_l)
    t |> equal(arr[at_pos + 1_l], uint8(0xFF))
    delete arr
}
