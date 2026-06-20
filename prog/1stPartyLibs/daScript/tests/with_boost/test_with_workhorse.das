options gen2
options indenting = 4

require dastest/testing_boost public
require daslib/with_boost

struct A {
    f1 : int
}

[test]
def test_int_array_default_name(t : T?) {
    //! `with_(arr[i]) { _ = X }` on a workhorse-element array — block
    //! param `_` is `int&`, mutation persists in the array.
    var arr <- [10, 20, 30]
    with_(arr[1]) {
        _ = 222
    }
    t |> equal(arr[0], 10, "untouched element stays")
    t |> equal(arr[1], 222, "workhorse mutation persists")
    t |> equal(arr[2], 30, "untouched element stays")
    delete arr
}

[test]
def test_int_array_named_binding(t : T?) {
    //! Named binding `$(x)` — same propagation, daslang infers `x : int&`
    //! from the helper sig.
    var arr <- [1, 2, 3]
    with_(arr[0]) $(x) {
        x = 999
    }
    t |> equal(arr[0], 999, "named workhorse binding mutation persists")
    delete arr
}

[test]
def test_float_array_named(t : T?) {
    var arr <- [1.0f, 2.0f, 3.0f]
    with_(arr[2]) $(x) {
        x = 99.5f
    }
    t |> equal(arr[2], 99.5f, "float workhorse mutation persists")
    delete arr
}

[test]
def test_multi_arg_workhorse(t : T?) {
    //! Multi-arg form across two workhorse arrays.
    var a <- [10, 20, 30]
    var b <- [100, 200, 300]
    with_(a[0], b[1]) $(x, y) {
        x = y + 1
    }
    t |> equal(a[0], 201, "x = y + 1 with workhorse types")
    t |> equal(b[1], 200, "source unchanged")
    delete a
    delete b
}

[test]
def test_mixed_workhorse_struct(t : T?) {
    var ints <- [10, 20]
    var structs <- [A(f1 = 100)]
    with_(ints[0], structs[0]) $(i, s) {
        i = s.f1 + 1
    }
    t |> equal(ints[0], 101, "workhorse + struct multi-arg")
    delete ints
    delete structs
}
