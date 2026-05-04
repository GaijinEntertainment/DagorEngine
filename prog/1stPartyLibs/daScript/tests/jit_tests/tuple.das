options gen2
require dastest/testing_boost

typedef FooT = tuple<a : int; b : float; c : string; d : int>
typedef BarT = tuple<a : int; foo : FooT; b : int>


def tuple_field(var f : FooT) {
    f.a = 1
    f.b = 2.0
    f.c = "hello"
    return f.d
}

def make_tuple_cmres : FooT {
    return (1, 0.0, "bar", 0)
}

def make_tuple_with_subtuple_cmres : BarT {
    return (1,  (2, 3., "", 0), 4)
}

def FooT : FooT {
    return (1234, 1234.0, "1234", 1234)
}

def make_tuple_with_field_call_to_cmres : BarT {
    return (1, FooT(), 4)
}

def make_tuple_on_stack(which : bool) : FooT {
    var a =  (1, 2.0, "3", 4)
    var b =  (2, 3.0, "4", 5)
    return which ? a : b
}

def make_tuple_with_subtuple_on_stack(which : bool) : BarT {
    var a =  (1,  (2, 3., "", 0), 4)
    var b =  (2,  (3, 4., "", 0), 5)
    return which ? a : b
}

[test]
def test_tuple(t : T?) {
    t |> run("tuple field") @(t : T?) {
        var f : FooT
        f.d = 13
        t |> success(!jit_enabled() || is_jit_function(@@tuple_field))
        var res = tuple_field(f)
        t |> equal(13, res)
        t |> equal(1, f.a)
        t |> equal(2.0, f.b)
        t |> equal("hello", f.c)
    }
    t |> run("make tuple cmres") @(t : T?) {
        var f = make_tuple_cmres()
        t |> success(!jit_enabled() || is_jit_function(@@make_tuple_cmres))
        t |> equal(1, f.a)
        t |> equal(0.0, f.b)
        t |> equal("bar", f.c)
        t |> equal(0, f.d)
    }
    t |> run("make struct with substruct cmres") @(t : T?) {
        var f = make_tuple_with_subtuple_cmres()
        t |> success(!jit_enabled() || is_jit_function(@@make_tuple_with_subtuple_cmres))
        t |> equal(1, f.a)
        t |> equal(2, f.foo.a)
        t |> equal(3., f.foo.b)
        t |> equal("", f.foo.c)
        t |> equal(0, f.foo.d)
        t |> equal(4, f.b)
    }
    t |> run("make struct with substruct equal to call to cmres") @(t : T?) {
        var f = make_tuple_with_field_call_to_cmres()
        t |> success(!jit_enabled() || is_jit_function(@@make_tuple_with_field_call_to_cmres))
        t |> equal(1, f.a)
        t |> equal(1234,  f.foo.a)
        t |> equal(1234., f.foo.b)
        t |> equal("1234", f.foo.c)
        t |> equal(1234,  f.foo.d)
        t |> equal(4, f.b)
    }
    t |> run("make struct on stack") @(t : T?) {
        var a = make_tuple_on_stack(true)
        var b = make_tuple_on_stack(false)
        t |> success(!jit_enabled() || is_jit_function(@@make_tuple_on_stack))
        t |> equal(1,   a.a)
        t |> equal(2.,  a.b)
        t |> equal("3", a.c)
        t |> equal(4,   a.d)
        t |> equal(2,   b.a)
        t |> equal(3.,  b.b)
        t |> equal("4", b.c)
        t |> equal(5,   b.d)
    }
    t |> run("make struct with substruct on stack") @(t : T?) {
        var a = make_tuple_with_subtuple_on_stack(true)
        var b = make_tuple_with_subtuple_on_stack(false)
        t |> success(!jit_enabled() || is_jit_function(@@make_tuple_with_subtuple_on_stack))
        t |> equal(1,  a.a)
        t |> equal(2,  a.foo.a)
        t |> equal(3., a.foo.b)
        t |> equal("", a.foo.c)
        t |> equal(0,  a.foo.d)
        t |> equal(4,  a.b)
        t |> equal(2,  b.a)
        t |> equal(3,  b.foo.a)
        t |> equal(4., b.foo.b)
        t |> equal("", b.foo.c)
        t |> equal(0,  b.foo.d)
        t |> equal(5,  b.b)
    }
}
