// options log = true // , log_infer_passes=true
require UnitTest

struct Foo
    a : int
    b : array<int>

struct Bar
    z : array<Foo>

[sideeffects]
def test_clone_complex
    var x, y : array<Bar>
    resize(x,2)
    for xx in x
        resize(xx.z,4)
        for xxx in xx.z
            xxx.a = 5
            resize(xxx.b,3)
            for t in range(3)
                xxx.b[t] = t
    y := x
    for xx in x
        for xxx in xx.z
            xxx.a = 6
            for t in range(3)
                xxx.b[t] = t + 1
    for yy in y
        for yyy in yy.z
            assert(yyy.a==5)
            for t in range(3)
                assert(yyy.b[t]==t)

[sideeffects]
def test_clone_struct
    var x, y : Foo
    x.a = 5
    resize(x.b,3)
    for t in range(3)
        x.b[t] = t
    y := x
    x.a = 6
    for t in range(3)
        x.b[t] = t + 1
    assert(y.a==5)
    for t in range(3)
        assert(y.b[t]==t)

[sideeffects]
def test_clone_array
    var x, y : array<int>
    resize(x,3)
    for t in range(3)
        x[t] = t
    y := x
    for t in range(3)
        x[t] = t + 1
    for t in range(3)
        verify(y[t]==t)

[sideeffects]
def test_clone_table
    var x, y : table<int;string>
    for t in range(3)
        x[t] = string(t)
    y := x
    for t in range(3)
        x[t] = string(t+1)
    assert(length(y)==3)
    for t in range(3)
        verify(y[t]==string(t))

[export]
def test
    test_clone_array()
    test_clone_table()
    test_clone_struct()
    test_clone_complex()
    return true

[export]
def test_if_compiles ( var a : dasvector`TestObjectFoo; b : TestObjectFoo# )
    push_clone(a,b)
