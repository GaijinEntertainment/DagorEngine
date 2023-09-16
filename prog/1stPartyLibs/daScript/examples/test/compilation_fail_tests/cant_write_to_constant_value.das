expect 30504:3

struct Foo
    i : int

struct Test
    v : Foo const? const

[export]
def test()
    let foo : Foo
    unsafe
        var t = [[Test v = addr(foo)]]
        t.v.i = 10                          // 30504: can't write to a constant value, int const& = int const
        var r = t.v
        r.i = 10                            // 30504: can't write to a constant value, int const& = int const
        var j = 10
        t.v?.i ?? j = 11                    // 30504: can't write to a constant value, int const& = int const
    return true
