// new type inference failures
// verifies compiler rejects `new` on non-struct types (via generic)
// and `new` on undefined type names
options gen2
// options log_infer_passes=true

expect 30109, 30301, 30101

struct FooBar {
    a, b : int
}

def oops(a : auto(SomeType)) {
    return new SomeType     // 30109: only tuples, variants, structures or handled types can be allocated on the heap, not 'int aka SomeType'
}

def test {
    new Foo                 // 30301: undefined new expression type 'Foo'
    oops(1)                 // 30101: cannot infer type for function call 'oops' with 'auto' return type
    let t : FooBar
    oops(t)
}
