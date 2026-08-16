options gen2
require dastest/testing_boost public

// Regression #3269 (bug F): an empty struct is 0 bytes standalone but occupies >=1 byte in C++
// (no zero-size member). Embedding one by value used to make the AOT-emitted sizeof/offsetof
// static_asserts diverge from the interpreter layout. Structure size/offset now count an
// empty-struct field as >=1, so the emitted C++ matches. The empty field is placed first to
// exercise the offsets of the fields that follow it.

struct Marker {}

struct Holder {
    m : Marker
    x : int
    y : int
}

[test]
def test_empty_struct_field_aot(t : T?) {
    t |> equal(typeinfo sizeof(type<Marker>), 0)
    let h = Holder(x = 7, y = 9)
    t |> equal(h.x, 7)
    t |> equal(h.y, 9)
}
