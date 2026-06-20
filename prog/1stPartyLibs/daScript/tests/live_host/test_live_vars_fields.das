options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require live/live_vars
require live_host
require daslib/archive

// --- Compilation smoke ---
//
// If the field-walking pass doesn't emit clean code, this file fails to
// compile. The generated `__before_reload_live_vars` / `__after_reload_live_vars`
// are called by the host (C++ findFunction lookup), not from daslang user code,
// so we can't invoke them directly here.

struct MixedFields {
    @live preserved_int : int
    @live preserved_str : string
    transient_int : int
    transient_str : string
}

var mixed_global : MixedFields  // non-@live; per-field preservation only

struct InnerState {
    @live keeper : int
    extra : int
}

var @live whole_global : InnerState  // @live on the global wins; whole struct preserved

[test]
def test_macro_emits_for_field_targets(t : T?) {
    t |> run("file with @live struct fields compiles") <| @(t : T?) {
        // Reaching this assertion proves the macro accepted both shapes:
        //   (1) non-@live global with @live fields  → per-field save/load
        //   (2) @live global with @live + non-@live fields → whole-struct save/load
        t |> success(true, "field-walking pass produced compilable code")
    }
}

// --- Per-field Archive round-trip ---
//
// Mirrors the byte sequence the generated per-field save/load blocks emit:
// `__live_vars_<mod>_<var>__<field>` keyed entries, each carrying an init-hash
// and one field value.

[test]
def test_per_field_archive_round_trip(t : T?) {
    t |> run("preserved_int round-trips via per-field key") <| @(t : T?) {
        let key = "__live_vars__mixed_global__preserved_int"
        let init_hash = int64(0)  // no init expr → hash 0

        mixed_global.preserved_int = 12345

        // Save (mirrors generated save block)
        var ser = new MemSerializer()
        var arch = Archive(reading = false, stream = ser)
        var save_hash = init_hash
        arch |> serialize(save_hash)
        arch |> serialize(mixed_global.preserved_int)
        let data <- ser->extractData()
        live_store_bytes(key, data)
        unsafe { delete ser; }

        // Reset
        mixed_global.preserved_int = 0

        // Load (mirrors generated load block)
        var load_data : array<uint8>
        if (live_load_bytes(key, load_data) && length(load_data) > 0) {
            var deser = new MemSerializer(load_data)
            var arch2 = Archive(reading = true, stream = deser)
            var stored_hash : int64
            arch2 |> serialize(stored_hash)
            if (stored_hash == init_hash) {
                arch2 |> serialize(mixed_global.preserved_int)
            }
            unsafe { delete deser; }
        }
        t |> equal(mixed_global.preserved_int, 12345)
    }

    t |> run("preserved_str round-trips via per-field key") <| @(t : T?) {
        let key = "__live_vars__mixed_global__preserved_str"
        let init_hash = int64(0)

        mixed_global.preserved_str := "kept across reload"

        var ser = new MemSerializer()
        var arch = Archive(reading = false, stream = ser)
        var save_hash = init_hash
        arch |> serialize(save_hash)
        arch |> serialize(mixed_global.preserved_str)
        let data <- ser->extractData()
        live_store_bytes(key, data)
        unsafe { delete ser; }

        mixed_global.preserved_str := ""

        var load_data : array<uint8>
        if (live_load_bytes(key, load_data) && length(load_data) > 0) {
            var deser = new MemSerializer(load_data)
            var arch2 = Archive(reading = true, stream = deser)
            var stored_hash : int64
            arch2 |> serialize(stored_hash)
            if (stored_hash == init_hash) {
                arch2 |> serialize(mixed_global.preserved_str)
            }
            unsafe { delete deser; }
        }
        t |> equal(mixed_global.preserved_str, "kept across reload")
    }

    t |> run("transient field is NOT under any per-field key") <| @(t : T?) {
        // Confirm the conventional key for a transient field is empty —
        // the macro deliberately did not emit a save block for it.
        let key = "__live_vars__mixed_global__transient_int"
        var load_data : array<uint8>
        let ok = live_load_bytes(key, load_data)
        // The host returns false (or empty) for never-stored keys.
        if (ok) {
            t |> equal(length(load_data), 0)
        } else {
            t |> success(true, "no entry for transient field, as expected")
        }
    }

    t |> run("hash mismatch on a field discards stale data") <| @(t : T?) {
        let key = "__live_vars__mixed_global__preserved_int_mismatch"
        let old_hash = int64(hash("123"))
        let new_hash = int64(hash("999"))  // simulates source default change

        var v = 5555
        var ser = new MemSerializer()
        var arch = Archive(reading = false, stream = ser)
        var h = old_hash
        arch |> serialize(h)
        arch |> serialize(v)
        let data <- ser->extractData()
        live_store_bytes(key, data)
        unsafe { delete ser; }

        var restored = 0
        var load_data : array<uint8>
        if (live_load_bytes(key, load_data) && length(load_data) > 0) {
            var deser = new MemSerializer(load_data)
            var arch2 = Archive(reading = true, stream = deser)
            var stored_hash : int64
            arch2 |> serialize(stored_hash)
            if (stored_hash == new_hash) {
                arch2 |> serialize(restored)
            }
            unsafe { delete deser; }
        }
        t |> equal(restored, 0)
    }

    t |> run("per-field hash combines field init AND global init (XOR)") <| @(t : T?) {
        // Mirrors the macro's hash composition. Source-level edits to either
        // the struct field's init or the global's initializer must invalidate
        // the cache. Both parts together = single per-field key.
        let key = "__live_vars__synthetic_combined_hash"

        let g_init_old = int64(hash("S(x = 100)"))
        let g_init_new = int64(hash("S(x = 999)"))
        let f_init = int64(hash("5"))

        let combined_old = g_init_old ^ f_init
        let combined_new = g_init_new ^ f_init  // same field, different global init

        var v = 7777
        var ser = new MemSerializer()
        var arch = Archive(reading = false, stream = ser)
        var h = combined_old
        arch |> serialize(h)
        arch |> serialize(v)
        let data <- ser->extractData()
        live_store_bytes(key, data)
        unsafe { delete ser; }

        // Reload as if source g.init changed: we expect combined_new.
        var restored = 0
        var load_data : array<uint8>
        if (live_load_bytes(key, load_data) && length(load_data) > 0) {
            var deser = new MemSerializer(load_data)
            var arch2 = Archive(reading = true, stream = deser)
            var stored_hash : int64
            arch2 |> serialize(stored_hash)
            if (stored_hash == combined_new) {
                arch2 |> serialize(restored)
            }
            unsafe { delete deser; }
        }
        t |> equal(restored, 0)  // editing g.init alone busts the per-field cache
    }
}
