options gen2
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require live/live_vars
require live_host
require daslib/archive

// --- Test globals tagged with @live ---

var @live test_int = 42
var @live test_float = 3.14
var @live test_string = "hello"
var @live test_bool = true
var @live test_array : array<float>

// Globals without @live — should NOT be serialized
var not_live_value = 999

[test]
def test_live_vars_compiles(t : T?) {
    t |> run("@live annotation compiles") @(t : T?) {
        // If the macro failed, this file wouldn't compile at all.
        // The macro generates __before_reload_live_vars and __after_reload_live_vars
        // which the host discovers by name prefix.
        t |> success(true, "@live globals compiled — macro generated serialization functions")
    }
}

[test]
def test_live_vars_archive_round_trip(t : T?) {
    t |> run("int round-trip via Archive") @(t : T?) {
        var original = 99
        var ser = new MemSerializer()
        var arch = Archive(reading = false, stream = ser)
        arch |> serialize(original)
        var data <- ser->extractData()
        unsafe { delete ser; }

        var restored = 0
        var deser = new MemSerializer(data)
        var arch2 = Archive(reading = true, stream = deser)
        arch2 |> serialize(restored)
        unsafe { delete deser; }

        t |> equal(restored, 99)
    }

    t |> run("string round-trip via Archive") @(t : T?) {
        var original = "test string"
        var ser = new MemSerializer()
        var arch = Archive(reading = false, stream = ser)
        arch |> serialize(original)
        var data <- ser->extractData()
        unsafe { delete ser; }

        var restored = ""
        var deser = new MemSerializer(data)
        var arch2 = Archive(reading = true, stream = deser)
        arch2 |> serialize(restored)
        unsafe { delete deser; }

        t |> equal(restored, "test string")
    }

    t |> run("array<float> round-trip via Archive") @(t : T?) {
        var original : array<float>
        original |> push(1.0)
        original |> push(2.0)
        original |> push(3.0)
        var ser = new MemSerializer()
        var arch = Archive(reading = false, stream = ser)
        arch |> serialize(original)
        var data <- ser->extractData()
        unsafe { delete ser; }

        var restored : array<float>
        var deser = new MemSerializer(data)
        var arch2 = Archive(reading = true, stream = deser)
        arch2 |> serialize(restored)
        unsafe { delete deser; }

        t |> equal(length(restored), 3)
        t |> equal(restored[0], 1.0)
        t |> equal(restored[1], 2.0)
        t |> equal(restored[2], 3.0)
    }

    t |> run("multiple vars round-trip via store") @(t : T?) {
        // Simulate what the generated __before_reload_live_vars does
        test_int = 77
        test_float = 1.414
        test_string = "preserved"
        test_bool = false
        test_array |> clear()
        test_array |> push(10.0)
        test_array |> push(20.0)

        var ser = new MemSerializer()
        var arch = Archive(reading = false, stream = ser)
        arch |> serialize(test_int)
        arch |> serialize(test_float)
        arch |> serialize(test_string)
        arch |> serialize(test_bool)
        arch |> serialize(test_array)
        var data <- ser->extractData()
        live_store_bytes("__test_live_vars_manual", data)
        unsafe { delete ser; }

        // Reset
        test_int = 0
        test_float = 0.0
        test_string = ""
        test_bool = true
        test_array |> clear()

        // Simulate what the generated __after_reload_live_vars does
        var load_data : array<uint8>
        let ok = live_load_bytes("__test_live_vars_manual", load_data)
        t |> success(ok, "load_bytes returned true")

        var deser = new MemSerializer(load_data)
        var arch2 = Archive(reading = true, stream = deser)
        arch2 |> serialize(test_int)
        arch2 |> serialize(test_float)
        arch2 |> serialize(test_string)
        arch2 |> serialize(test_bool)
        arch2 |> serialize(test_array)
        unsafe { delete deser; }

        t |> equal(test_int, 77)
        t |> equal(test_float, 1.414)
        t |> equal(test_string, "preserved")
        t |> equal(test_bool, false)
        t |> equal(length(test_array), 2)
        t |> equal(test_array[0], 10.0)
        t |> equal(test_array[1], 20.0)
    }

    t |> run("garbage data does not crash with try/recover") @(t : T?) {
        var garbage : array<uint8>
        garbage |> push(uint8(0xFF))
        garbage |> push(uint8(0xFE))
        live_store_bytes("__test_garbage", garbage)

        var load_data : array<uint8>
        if (live_load_bytes("__test_garbage", load_data) && length(load_data) > 0) {
            try {
                var deser = new MemSerializer(load_data)
                var arch = Archive(reading = true, stream = deser)
                var v : int
                arch |> serialize(v)
                var s = ""
                arch |> serialize(s)
                unsafe { delete deser; }
            } recover {
                // Expected — garbage data causes deserialization failure
            }
        }
        t |> success(true, "survived garbage data without crash")
    }
}

[test]
def test_live_vars_per_key_round_trip(t : T?) {
    t |> run("per-variable save/load with matching hash") <| @(t : T?) {
        // Simulate what the generated code does: hash + value
        let init_hash = int64(hash("42"))  // hash of describe(test_int.init) = "42"
        let key = "__test_per_key_int"
        // Save
        var value = 999
        var ser = new MemSerializer()
        var arch = Archive(reading = false, stream = ser)
        var h = init_hash
        arch |> serialize(h)
        arch |> serialize(value)
        var data <- ser->extractData()
        live_store_bytes(key, data)
        unsafe { delete ser; }
        // Load with same hash — should restore
        var restored = 0
        var load_data : array<uint8>
        if (live_load_bytes(key, load_data) && length(load_data) > 0) {
            var deser = new MemSerializer(load_data)
            var arch2 = Archive(reading = true, stream = deser)
            var stored_hash : int64
            arch2 |> serialize(stored_hash)
            if (stored_hash == init_hash) {
                arch2 |> serialize(restored)
            }
            unsafe { delete deser; }
        }
        t |> equal(restored, 999)
    }
    t |> run("hash mismatch discards stale data") <| @(t : T?) {
        let key = "__test_per_key_mismatch"
        let old_hash = int64(hash("42"))
        let new_hash = int64(hash("99"))  // init changed from 42 to 99
        // Save with old hash
        var value = 777
        var ser = new MemSerializer()
        var arch = Archive(reading = false, stream = ser)
        var h = old_hash
        arch |> serialize(h)
        arch |> serialize(value)
        var data <- ser->extractData()
        live_store_bytes(key, data)
        unsafe { delete ser; }
        // Load expecting new hash — should NOT restore
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
    t |> run("no-init var uses hash 0") <| @(t : T?) {
        let key = "__test_per_key_no_init"
        let no_init_hash = int64(0)
        // Save with hash 0
        var value = 55
        var ser = new MemSerializer()
        var arch = Archive(reading = false, stream = ser)
        var h = no_init_hash
        arch |> serialize(h)
        arch |> serialize(value)
        var data <- ser->extractData()
        live_store_bytes(key, data)
        unsafe { delete ser; }
        // Load with hash 0 — should restore
        var restored = 0
        var load_data : array<uint8>
        if (live_load_bytes(key, load_data) && length(load_data) > 0) {
            var deser = new MemSerializer(load_data)
            var arch2 = Archive(reading = true, stream = deser)
            var stored_hash : int64
            arch2 |> serialize(stored_hash)
            if (stored_hash == no_init_hash) {
                arch2 |> serialize(restored)
            }
            unsafe { delete deser; }
        }
        t |> equal(restored, 55)
    }
}
