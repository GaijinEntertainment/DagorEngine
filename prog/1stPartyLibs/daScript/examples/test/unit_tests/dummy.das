options gen2
require UnitTest

struct MyStruct {
    a : int8
    @safe_when_uninitialized b : SomeDummyType   // { int; float }
}

[export]
def test {
    assert(typeinfo sizeof(type<MyStruct>) == 12)
    assert(typeinfo alignof(type<MyStruct>) == 4)
    return true
}
