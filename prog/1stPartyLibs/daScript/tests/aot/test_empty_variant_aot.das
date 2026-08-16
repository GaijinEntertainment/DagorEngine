options gen2
require dastest/testing_boost public

// Regression #3269 (bug B): an empty variant has no alternatives, only its 4-byte index tag.
// The AOT emitter used to write TVariant<0,4,> (trailing comma + size 0 -> char data[-4]). It now
// emits TVariant<4,4> (the variant size counts the always-present index tag) and a dedicated
// empty-variant C++ specialization compiles it without a zero-length data array. The empty variant
// is also embedded by value to exercise the containing struct's layout.

variant Empty {}

struct Holder {
    e : Empty
    x : int
}

[test]
def test_empty_variant_aot(t : T?) {
    t |> equal(typeinfo sizeof(type<Empty>), 4)
    let h = Holder(x = 13)
    t |> equal(h.x, 13)
}
