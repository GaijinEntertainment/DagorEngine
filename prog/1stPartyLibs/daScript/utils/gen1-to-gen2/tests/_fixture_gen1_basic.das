options gen2 = false

// a header comment, which the conversion must not move or lose

[safe_when_uninitialized]
struct Foo
    a : int = 5

def make_it()
    var s = [[Foo /*keep*/ a=1]]
    return s.a
