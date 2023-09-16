expect 30501:5    // cant_dereference

struct Bar
    c:int

struct Foo
    a:int
    b:Bar?

def test
    var a:int          // can't dereference not a pointer
    debug(deref(a))
    var b:void?        // can't dereference void pointer
    debug(deref(b))
    var c:int
    debug(c ?? 0)   // can't dereference non-pointer
    var d:void?
    debug(d ?? 0)   // can't dereference void pointer
    var e:int?
    debug(e ?? 1.0) // default value type mismatch


