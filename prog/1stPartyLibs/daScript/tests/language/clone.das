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
    var x <- {for (ii in range(3)); ii => string(ii)}
    var y : table<int; string>
    y := x
    for (ii in range(3)) {
        x |> insert(ii, string(ii + 1))
    }
    t |> equal(length(y), 3)
    for (ii in range(3)) {
        t |> equal(y |> get_value(ii), string(ii))
    }
}

// custom clone overloads on a struct: regression for synthesized
// tuple/variant clone constness propagation. The outer `:= ` on the
// tuple must propagate the RHS constness to each field, so the user's
// const-flavor clone fires for `let` (const) tuples and the non-const
// flavor fires for `var` (non-const) tuples.
struct CFoo {
    a : int
    b : array<int>
    via : int   // 1 = const path, 2 = non-const path
}

def clone(var res : CFoo; var src : CFoo ==const) {
    res.a = src.a
    res.b := src.b
    res.via = 2
}

def clone(var res : CFoo; src : CFoo) {
    res.a = src.a
    res.b := src.b
    res.via = 1
}

[sideeffects]
def test_clone_tuple_constness_dispatch(t : T?) {
    let tup1 = (10, CFoo(a = 1))
    var tup2 = (20, CFoo(a = 2))
    let c1 := tup1
    let c2 := tup2
    t |> equal(c1._0, 10)
    t |> equal(c1._1.a, 1)
    t |> equal(c1._1.via, 1)
    t |> equal(c2._0, 20)
    t |> equal(c2._1.a, 2)
    t |> equal(c2._1.via, 2)
}

variant VFoo {
    a : array<string>
    b : float
}

typedef TBar = tuple<a : array<string>; b : float>

struct SHolder {
    f : CFoo
}

def make_vfoo(arr : array<string>) : VFoo {
    return VFoo(a := arr)
}

def make_tbar(arr : array<string>) : TBar {
    return TBar(a := arr, b = 1.5)
}

def make_generic(proto : auto(TT); arr : array<string>) : TT -const {
    return <- TT(a := arr, b = 2.5)
}

def take_named(a : array<string>) : int {
    return length(a)
}

[sideeffects]
def test_clone_init_field(t : T?) {
    var arr <- ["x", "y"]
    let vf <- make_vfoo(arr)
    t |> equal(length(arr), 2)
    t |> success(vf is a)
    t |> equal(length(vf as a), 2)
    arr[0] = "z"
    t |> equal((vf as a)[0], "x")
    let vb <- VFoo(b := 1)
    t |> equal(vb as b, 1.0)
    let ev <- variant type<VFoo>(a := arr)
    t |> equal(length(arr), 2)
    t |> equal((ev as a)[0], "z")
    let tb <- make_tbar(arr)
    t |> equal(length(arr), 2)
    t |> equal(length(tb.a), 2)
    t |> equal(tb.b, 1.5)
    arr[1] = "w"
    t |> equal(tb.a[1], "y")
    let proto : TBar
    let tg <- make_generic(proto, arr)
    t |> equal(length(arr), 2)
    t |> equal(tg.a[1], "w")
    t |> equal(tg.b, 2.5)
    let td : TBar[2] <- fixed_array(TBar(a := arr, b = 3.0), TBar(a := arr, b = 4.0))
    t |> equal(length(arr), 2)
    t |> equal(td[1].a[0], "z")
    t |> equal(td[1].b, 4.0)
    t |> equal(take_named(a := arr), 2)
    t |> equal(length(arr), 2)
    let src = CFoo(a = 7)
    let sh <- SHolder(f := src)
    t |> equal(sh.f.a, 7)
    t |> equal(sh.f.via, 1)
    var vsrc = CFoo(a = 8)
    let sh2 <- SHolder(f := vsrc)
    t |> equal(sh2.f.a, 8)
    t |> equal(sh2.f.via, 2)
}

[test]
def test_clone(t : T?) {
    test_clone_array(t)
    test_clone_table(t)
    test_clone_struct(t)
    test_clone_complex(t)
    test_clone_tuple_constness_dispatch(t)
    test_clone_init_field(t)
}

[export]
def test_if_compiles(var a : dasvector`TestObjectFoo; b : TestObjectFoo#) {
    push_clone(a, b)
}
