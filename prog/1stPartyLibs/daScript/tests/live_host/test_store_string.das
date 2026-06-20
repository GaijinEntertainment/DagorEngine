options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require live_host

// ============================================================
//  live_store_string / live_load_string — string sibling of the
//  bytes-typed persistent store. Used by the LiveVarsPassMacro
//  JSON-text pivot (PR3) and the dasImgui widget serializer rail.
// ============================================================

[test]
def test_store_and_load_string(t : T?) {
    t |> run("store and load short string") @(t : T?) {
        live_store_string("string_key", "hello world")
        var loaded : string
        let ok = live_load_string("string_key", loaded)
        t |> success(ok, "load_string should return true")
        t |> equal(loaded, "hello world")
    }
    t |> run("load missing key returns false") @(t : T?) {
        var loaded : string
        let ok = live_load_string("nonexistent_string_xyz", loaded)
        t |> success(!ok, "load_string for missing key should return false")
    }
    t |> run("overwrite existing key") @(t : T?) {
        live_store_string("string_overwrite", "first")
        live_store_string("string_overwrite", "second")
        var loaded : string
        let ok = live_load_string("string_overwrite", loaded)
        t |> success(ok)
        t |> equal(loaded, "second")
    }
    t |> run("store empty string") @(t : T?) {
        live_store_string("string_empty", "")
        var loaded : string
        let ok = live_load_string("string_empty", loaded)
        t |> success(ok, "load_string for empty string should return true")
        t |> equal(loaded, "")
    }
    t |> run("store JSON-like content") @(t : T?) {
        let json = "\{\"a\":42,\"b\":\"hello\"\}"
        live_store_string("string_json", json)
        var loaded : string
        let ok = live_load_string("string_json", loaded)
        t |> success(ok)
        t |> equal(loaded, json)
    }
    t |> run("multiple independent string keys") @(t : T?) {
        live_store_string("string_multi_a", "alpha")
        live_store_string("string_multi_b", "beta")
        var la : string
        var lb : string
        live_load_string("string_multi_a", la)
        live_load_string("string_multi_b", lb)
        t |> equal(la, "alpha")
        t |> equal(lb, "beta")
    }
    t |> run("bytes and strings live in separate stores") @(t : T?) {
        // Same key in both stores — must not collide.
        var data : array<uint8>
        data |> push(uint8(42))
        live_store_bytes("shared_key", data)
        live_store_string("shared_key", "string_value")
        var loaded_bytes : array<uint8>
        var loaded_string : string
        let ok1 = live_load_bytes("shared_key", loaded_bytes)
        let ok2 = live_load_string("shared_key", loaded_string)
        t |> success(ok1, "bytes side intact")
        t |> success(ok2, "string side intact")
        t |> equal(length(loaded_bytes), 1)
        t |> equal(int(loaded_bytes[0]), 42)
        t |> equal(loaded_string, "string_value")
    }
}
