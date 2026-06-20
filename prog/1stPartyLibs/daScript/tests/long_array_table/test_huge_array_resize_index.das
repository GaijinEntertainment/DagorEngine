options gen2
options persistent_heap = true   // >4 GB arrays need PersistentHeapAllocator

require dastest/testing_boost public
require daslib/fio

// Memory-gated probe. Allocates a 5 GB `array<uint8>` and exercises
// int64 indexing at index 0 and index N-1 (a high position that requires
// `uint64_t(idx)*uint64_t(stride) + offset` for the address math).
//
// Skipped silently unless:
//   * running a 64-bit daslang (static_if on pointer size)
//   * DASLANG_HUGE_HEAP_TESTS=1 in the environment
//
// dastest auto-discovers this file (filename starts with `test_`), but the
// inline `huge_enabled()` gate silent-returns when the env var is unset, so
// CI sees a no-op pass. Run manually with the env var to actually allocate:
//   $env:DASLANG_HUGE_HEAP_TESTS = "1"
//   bin/Release/daslang.exe dastest/dastest.das -- --test tests/long_array_table/test_huge_array_resize_index.das

def huge_enabled() : bool {
    static_if (typeinfo sizeof(type<int?>) < 8) {
        return false
    }
    return has_env_variable("DASLANG_HUGE_HEAP_TESTS") && get_env_variable("DASLANG_HUGE_HEAP_TESTS") == "1"
}

[test]
def test_huge_array_resize_round_trip(t : T?) {
    if (!huge_enabled()) return
    let N = 5_l * 1024_l * 1024_l * 1024_l  // 5 GB elements of uint8
    var arr : array<uint8>
    arr |> resize(N)
    t |> equal(long_length(arr), N)
    arr[0_l] = uint8(0xC0)
    arr[N - 1_l] = uint8(0xDE)
    t |> equal(arr[0_l], uint8(0xC0))
    t |> equal(arr[N - 1_l], uint8(0xDE))
    delete arr
}

[test]
def test_huge_array_length_panics_long_length_ok(t : T?) {
    if (!huge_enabled()) return
    // 2.2 GB > INT_MAX (2.147 GB). long_length is safe; length() should panic
    // per the surface contract added in PR #2746.
    let N = 2200_l * 1024_l * 1024_l
    var arr : array<uint8>
    arr |> resize(N)
    t |> equal(long_length(arr), N)
    // length(arr) here should panic — but try/recover is banned for panic UX testing
    // (feedback_no_try_recover_for_soft_fail), so we don't assert the panic here.
    // Reproduce manually by uncommenting:
    //   let _l = length(arr)
    delete arr
}
