options gen2
require dastest/testing_boost public

[safe_when_uninitialized]
struct Bar {
    t : string = "blah"
    ta : array<float> <- make_10()
}

def make_10 {
    var a : array<float>
    for (i in range(10)) {
        a |> push(float(i))
    }
    return <- a
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
    binary_save(f0) <| $(data) {
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
