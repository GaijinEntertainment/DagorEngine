options gen2
options persistent_heap
require dastest/testing_boost public

[safe_when_uninitialized]
struct Bar {
    t : string = "blah"
    ta : array<float> <- make_10()
}

def make_10 {
    return <- [for (i in range(10)); float(i)]
}

[safe_when_uninitialized]
struct Foo {
    data_bool : bool = true
    data_int : int = 1
    data_float : float = 3.14
    data_bar : Bar <- Bar()
    data_uint_3 : uint[3] = fixed_array<uint>(1u, 2u, 3u)
}

[test]
def test_bin_serializer(t : T?) {
    let f0 <- Foo()
    var f1 : Foo
    binary_save(f0) $(data) {
        binary_load(f1, data)
        t |> equal(f1.data_bool, true)
        t |> equal(f1.data_int, 1)
        t |> equal(f1.data_float, 3.14)
        t |> equal(f1.data_bar.t, "blah")
        t |> success(f1.data_uint_3[0] == 1u && f1.data_uint_3[1] == 2u && f1.data_uint_3[2] == 3u)
        for (i in range(10)) {
            t |> equal(f1.data_bar.ta[i], float(i))
        }
    }
}

//! Keep the embedded fixed array so this fixture exercises differing array allocation strides.
struct BinaryStrideRecord { bytes : array<uint8>; values : float4[3] }
struct BinaryStrideEnvelope { records : array<BinaryStrideRecord> }
struct BinarySaveBuffer { bytes : array<uint8> }
[test]
def test_binary_array_element_stride(t : T?){
    var source : BinaryStrideEnvelope; source.records |> resize(3)
    for (record, i in source.records, count()){
        record.bytes |> resize(7 + i)
        for (value, k in record.bytes, count()){value = uint8(k + i)}
        record.values[2] = float4(float(i), 2.0, 3.0, 4.0)
    }
    binary_save(source) $(data){
        for (_repeat in range(8)){
            var decoded : BinaryStrideEnvelope
            binary_load(decoded, data)
            t |> equal(hash(decoded), hash(source), "nested arrays deserialize with the element stride")
            delete decoded
        }
    }
    delete source
}

[test]
def test_binary_save_reclaims_temporary_buffer(t : T?){
    var source : BinarySaveBuffer; source.bytes |> resize(64 * 1024)
    for (value, i in source.bytes, count()){value = uint8(i)}
    binary_save(source) $(data){let _size = length(data)}
    let before = heap_bytes_allocated()
    for (_repeat in range(16)){
        binary_save(source) $(data){let _size = length(data)}
    }
    let after = heap_bytes_allocated()
    t |> equal(after, before, "binary_save releases each temporary serialization buffer")
    delete source
}

[test]
def test_binary_save_reclaims_buffer_when_callback_panics(t : T?) {
    var source : BinarySaveBuffer; source.bytes |> resize(64 * 1024)
    try {
        binary_save(source) $(data) {let _size = length(data); panic("callback failure")}
    } recover {}
    let before = heap_bytes_allocated()
    var caught = 0
    for (_repeat in range(16)) {
        try {
            binary_save(source) $(data) {let _size = length(data); panic("callback failure")}
        } recover {caught++}
    }
    let after = heap_bytes_allocated()
    t |> equal(caught, 16, "callback exceptions propagate to the caller")
    t |> equal(after, before, "callback exceptions release the temporary serialization buffer")
    delete source
}

struct WithLambda {
    fn : lambda<(x : int) : int>
}

[test]
def test_bin_serializer_null_lambda(t : T?) {
    var w : WithLambda
    var saved = false
    let error_text = panic_text() {
        binary_save(w) $(data) {
            saved = true
        }
    }
    t |> equal(saved, false)
    t |> equal(error_text, "binary serialization of null lambdas is not supported")
}
