options gen2
require UnitTest
require dastest/testing_boost public

struct MyStruct {
    a : int8
    @safe_when_uninitialized b : SomeDummyType   // { int; float }
}

[test]
def test_dummy(t : T?) {
    t |> equal(typeinfo sizeof(type<MyStruct>), 12)
    t |> equal(typeinfo alignof(type<MyStruct>), 4)
}
