options gen2
require dastest/testing_boost public
require daslib/debugger

// These tests guard against drift between the daslib DapiArray/DapiTable
// debug mirrors (daslib/debugger.das) and the C++ das::Array/das::Table
// structs (include/daScript/misc/arraytype.h). The C++ side has a
// static_assert canary in daScriptC.cpp; this is the daslib-side mirror.
//
// The pattern: overlay a real array<T> / table<K;V> with a DapiArray? /
// DapiTable? pointer and verify size/capacity/tombstones read back the
// values daslang reports through its own API. Any field-offset drift —
// missing pad, wrong width, base-trailing-pad reuse difference between
// ABIs — would surface as a mismatched compare.

[test]
def test_dapi_array_size_capacity_match(t : T?) {
    var arr : array<int>
    arr |> resize(int64(7))
    let view = unsafe(reinterpret<DapiArray?>(unsafe(addr(arr))))
    t |> equal(view.size, uint64(7))
    t |> equal(view.capacity, uint64(long_capacity(arr)))
}

[test]
def test_dapi_array_data_pointer_non_null(t : T?) {
    var arr : array<int>
    arr |> resize(int64(3))
    let view = unsafe(reinterpret<DapiArray?>(unsafe(addr(arr))))
    t |> success(view.data != null)
}

[test]
def test_dapi_table_size_tombstones_match(t : T?) {
    var tab : table<int; int>
    tab |> insert(1, 100)
    tab |> insert(2, 200)
    tab |> insert(3, 300)
    let view = unsafe(reinterpret<DapiTable?>(unsafe(addr(tab))))
    t |> equal(view.size, uint64(3))
    t |> equal(view.tombstones, uint64(0))
}

[test]
def test_dapi_table_keys_hashes_non_null(t : T?) {
    // If the inherited DapiArray slice has the wrong size, keys/hashes
    // would land at the wrong offsets and either read garbage or null.
    var tab : table<int; int>
    tab |> insert(42, 100)
    let view = unsafe(reinterpret<DapiTable?>(unsafe(addr(tab))))
    t |> success(view.keys != null)
    t |> success(view.hashes != null)
}

[test]
def test_dapi_table_tombstones_after_erase(t : T?) {
    // Small (packed) tables erase via swap-remove and leave no tombstone; tombstones only
    // exist in the open-addressed regime, so grow past maxLinearCapacity (8) first.
    var tab : table<int; int>    // nolint:STYLE027  built via real insert path then erased to exercise tombstone layout
    for (i in range(10)) {
        tab |> insert(i, i * 100)
    }
    tab |> erase(3)
    let view = unsafe(reinterpret<DapiTable?>(unsafe(addr(tab))))
    t |> equal(view.size, uint64(9))
    t |> equal(view.tombstones, uint64(1))
}

[test]
def test_dapi_table_keys_offset_matches_runtime(t : T?) {
    // Catches base-trailing-pad drift on 32-bit Itanium ABI directly.
    // In the C++ Table memory layout, `keys` points to `data + capacity * sizeof(value)`.
    // If DapiArray's pad/size disagrees with das::Array's, DapiTable.keys reads from
    // the wrong slot and the equality below fails — independent of platform.
    var tab : table<int; int>    // nolint:STYLE027  built via real insert path to validate keys-offset layout
    for (i in range(8)) {
        tab |> insert(i, i * 10)
    }
    let view = unsafe(reinterpret<DapiTable?>(unsafe(addr(tab))))
    let data_addr = intptr(view.data)
    let expected_keys_addr = data_addr + view.capacity * uint64(4)   // value type = int = 4 bytes
    t |> equal(intptr(view.keys), expected_keys_addr)
}
