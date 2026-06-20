options gen2
options persistent_heap = true   // ~4.4 GB array<int> needs PersistentHeapAllocator

require dastest/testing_boost public
require daslib/fio

// Memory-gated probe targeting the SimNode_ArrayAt offset-math widening.
// `array<int>` with stride=4 and ~1.1G elements puts `idx * stride` above
// UINT32_MAX, so the address computation MUST be `uint64_t(idx) *
// uint64_t(stride) + offset` (per Phase 2/runtime_array.h). A uint32
// regression would wrap and read the wrong cache line — usually a segfault,
// occasionally a silently-wrong value.
//
// ~4.3 GB allocation. Skipped silently unless DASLANG_HUGE_HEAP_TESTS=1 on
// a 64-bit build.

def huge_enabled() : bool {
    static_if (typeinfo sizeof(type<int?>) < 8) {
        return false
    }
    return has_env_variable("DASLANG_HUGE_HEAP_TESTS") && get_env_variable("DASLANG_HUGE_HEAP_TESTS") == "1"
}

// (UINT32_MAX / 4) + ~32M slack = 1.1G elements; idx*stride > 4 GB at the tail.
let STRIDE_OVERFLOW_N = 1_100_000_000_l

[test]
def test_int_array_high_index_int64(t : T?) {
    if (!huge_enabled()) return
    var arr : array<int>
    arr |> resize(STRIDE_OVERFLOW_N)
    t |> equal(long_length(arr), STRIDE_OVERFLOW_N)
    // Write to a position where idx*4 > UINT32_MAX, then read it back.
    // If the address math is uint32, we either segfault or read garbage.
    let high = STRIDE_OVERFLOW_N - 1_l
    arr[high] = int(0x12345678)
    arr[0_l] = int(0x0BADC0DE)
    t |> equal(arr[high], int(0x12345678))
    t |> equal(arr[0_l], int(0x0BADC0DE))
    delete arr
}

[test]
def test_int_array_high_index_uint64(t : T?) {
    if (!huge_enabled()) return
    var arr : array<int>
    arr |> resize(STRIDE_OVERFLOW_N)
    // Same test, uint64 index instead of int64 — exercises the U64 SimNode
    // variant's offset math.
    let high : uint64 = uint64(STRIDE_OVERFLOW_N) - 1_ul
    arr[high] = int(0x12345678)
    arr[0_ul] = int(0x0BADC0DE)
    t |> equal(arr[high], int(0x12345678))
    t |> equal(arr[0_ul], int(0x0BADC0DE))
    delete arr
}
