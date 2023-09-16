struct Foo
    a : string

def operator [] ( t:Foo; cmp:string )
    return "{t . . a}-{cmp}"

def operator ?[] ( t:Foo; cmp:string ) : string const?#
    if cmp==t . . a
        unsafe
            return reinterpret<string const?#> addr(t . . a)
    else
        unsafe
            return reinterpret<string const?#> null

def operator . ( t:Foo; name:string )
    return [[Foo a="{t . . a}.{name}"]]

def operator ?. ( t:Foo; name:string ) : string const?#
    if name=="yes"
        unsafe
            return reinterpret<string const?#> addr(t . . a)
    else
        unsafe
            return reinterpret<string const?#> null
[export]
def test
    var t = [[Foo a="hello"]]
    var z = t["blah"]
    var w = t?["blah"] ?? "not_blah"
    var W = t?["hello"] ?? "this???"
    var EZ = t.fooBar
    var HARD_YES = t?.yes ?? "how??"
    var HARD_NO = t?.no ?? "hard no"
    assert(z=="hello-blah")
    assert(w=="not_blah")
    assert(W=="hello")
    assert(EZ . . a=="hello.fooBar")
    assert(HARD_YES=="hello")
    assert(HARD_NO=="hard no")
    return true