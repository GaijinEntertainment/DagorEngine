options no_unused_function_arguments = false

struct Foo
    a,b : int

struct Bar
    c, d : int

[sideeffects]
def setA(var obj;val:auto(valT))
    if typeinfo(has_field<a> obj)
        if typeinfo(typename obj.a) == typeinfo(typename type<valT -const>)
            obj.a = val

let
    canQQ : bool const = false

[export]
def test
    var f : Foo
    var b : Bar
    setA(f,1)
    setA(b,2)
    assert(f.a == 1)
    if canQQ
        broken = badly  // this will be compiled out, so as long as there are no syntax errors...
    if 2 + 2 == 4
        static_assert(true,"nothing to look at")
    else
        badly = broken  // this will be compiled out, so as long as there are no syntax errors...
    return true

