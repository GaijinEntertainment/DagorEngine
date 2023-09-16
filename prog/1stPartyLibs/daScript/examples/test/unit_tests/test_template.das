require daslib/templates

[template (a), sideeffects]
def boo ( x : int; a : auto(TT) )
    return "{x}_{typeinfo(typename type<TT>)}"

[export]
def test
    var a = boo ( 123, type<float> )
    assert(a == "123_float const")
    var b = boo ( 456, decltype(2+2) )
    assert(b == "456_int const")
    return true
