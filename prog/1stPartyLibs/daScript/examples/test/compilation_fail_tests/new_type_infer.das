// options log_infer_passes=true

expect 30109, 30301

struct FooBar
    a,b:int

def oops(a:auto(SomeType))
    return new SomeType     // 30109: can only new structures or handles

def test
    new Foo                 // 30301: undefined type Foo
    oops(1)
    let t: FooBar
    oops(t)
