options gen2
// options log = true // , log_infer_passes=true
require UnitTest
require dastest/testing_boost public

struct Foo {
    a : int
    b : array<int>
}

struct Bar {
    z : array<Foo>
}

[sideeffects]
def test_clone_complex(t : T?) {
    var x, y : array<Bar>
    resize(x, 2)
    for (xx in x) {
        resize(xx.z, 4)
        for (xxx in xx.z) {
            xxx.a = 5
            resize(xxx.b, 3)
            for (ii in range(3)) {
                xxx.b[ii] = ii
            }
        }
    }
    y := x
    for (xx in x) {
        for (xxx in xx.z) {
            xxx.a = 6
            for (ii in range(3)) {
                xxx.b[ii] = ii + 1
            }
        }
    }
    for (yy in y) {
        for (yyy in yy.z) {
            t |> equal(yyy.a, 5)
            for (ii in range(3)) {
                t |> equal(yyy.b[ii], ii)
            }
        }
    }
}

[sideeffects]
def test_clone_struct(t : T?) {
    var x, y : Foo
    x.a = 5
    resize(x.b, 3)
    for (ii in range(3)) {
        x.b[ii] = ii
    }
    y := x
    x.a = 6
    for (ii in range(3)) {
        x.b[ii] = ii + 1
    }
    t |> equal(y.a, 5)
    for (ii in range(3)) {
        t |> equal(y.b[ii], ii)
    }
}

[sideeffects]
def test_clone_array(t : T?) {
    var x, y : array<int>
    resize(x, 3)
    for (ii in range(3)) {
        x[ii] = ii
    }
    y := x
    for (ii in range(3)) {
        x[ii] = ii + 1
    }
    for (ii in range(3)) {
        t |> equal(y[ii], ii)
    }
}

[sideeffects]
def test_clone_table(t : T?) {
    var x, y : table<int; string>
    for (ii in range(3)) {
        x |> insert(ii, string(ii))
    }
    y := x
    for (ii in range(3)) {
        x |> insert(ii, string(ii + 1))
    }
    t |> equal(length(y), 3)
    for (ii in range(3)) {
        t |> equal(y |> get_value(ii), string(ii))
    }
}

[test]
def test_clone(t : T?) {
    test_clone_array(t)
    test_clone_table(t)
    test_clone_struct(t)
    test_clone_complex(t)
}

[export]
def test_if_compiles(var a : dasvector`TestObjectFoo; b : TestObjectFoo#) {
    push_clone(a, b)
}
