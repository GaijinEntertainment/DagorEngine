options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require live_host

[test]
def test_store_and_load(t : T?) {
    t |> run("store and load bytes") @(t : T?) {
        var data : array<uint8>
        data |> push(uint8(1))
        data |> push(uint8(2))
        data |> push(uint8(3))
        live_store_bytes("test_key", data)
        var loaded : array<uint8>
        let ok = live_load_bytes("test_key", loaded)
        t |> success(ok, "load_bytes should return true")
        t |> equal(length(loaded), 3)
        t |> equal(int(loaded[0]), 1)
        t |> equal(int(loaded[1]), 2)
        t |> equal(int(loaded[2]), 3)
    }
    t |> run("load missing key returns false") @(t : T?) {
        var loaded : array<uint8>
        let ok = live_load_bytes("nonexistent_key_xyz", loaded)
        t |> success(!ok, "load_bytes for missing key should return false")
        t |> equal(length(loaded), 0)
    }
    t |> run("overwrite existing key") @(t : T?) {
        var data1 : array<uint8>
        data1 |> push(uint8(10))
        live_store_bytes("overwrite_key", data1)
        var data2 : array<uint8>
        data2 |> push(uint8(20))
        data2 |> push(uint8(30))
        live_store_bytes("overwrite_key", data2)
        var loaded : array<uint8>
        let ok = live_load_bytes("overwrite_key", loaded)
        t |> success(ok)
        t |> equal(length(loaded), 2)
        t |> equal(int(loaded[0]), 20)
        t |> equal(int(loaded[1]), 30)
    }
    t |> run("store empty array") @(t : T?) {
        var data : array<uint8>
        live_store_bytes("empty_key", data)
        var loaded : array<uint8>
        let ok = live_load_bytes("empty_key", loaded)
        t |> success(ok, "load_bytes for empty data should return true")
        t |> equal(length(loaded), 0)
    }
    t |> run("store large data") @(t : T?) {
        var data : array<uint8>
        for (i in range(10000)) {
            data |> push(uint8(i % 256))
        }
        live_store_bytes("large_key", data)
        var loaded : array<uint8>
        let ok = live_load_bytes("large_key", loaded)
        t |> success(ok)
        t |> equal(length(loaded), 10000)
        var all_match = true
        for (i in range(10000)) {
            if (int(loaded[i]) != i % 256) {
                all_match = false
                break
            }
        }
        t |> success(all_match, "large data should round-trip correctly")
    }
    t |> run("multiple independent keys") @(t : T?) {
        var a : array<uint8>
        a |> push(uint8(11))
        live_store_bytes("multi_a", a)
        var b : array<uint8>
        b |> push(uint8(22))
        live_store_bytes("multi_b", b)
        var la : array<uint8>
        var lb : array<uint8>
        live_load_bytes("multi_a", la)
        live_load_bytes("multi_b", lb)
        t |> equal(int(la[0]), 11)
        t |> equal(int(lb[0]), 22)
    }
}
