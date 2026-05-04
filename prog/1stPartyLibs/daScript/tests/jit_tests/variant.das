options gen2
require dastest/testing_boost

variant FooT {
    a : int
    b : float
    c : string
}

variant BarT {
    a : int
    foo : FooT
}

def variant_field(var f : FooT) {
    unsafe {
        f.b = 1.
        return f.a
    }
}

def variant_field_move(var f : FooT) {
    var b : BarT
    b = BarT(foo <- f)
    return b
}

def variant_is(var f : FooT) {
    f = FooT(b = 1.)
    return f is b
}

def variant_as(var f : FooT) {
    f = FooT(b = 1.)
    f as b = 2.
    return f as b
}

def variant_as_panic(var f : FooT) {
    f = FooT(b = 1.)
    return f as a
}

def make_variant_cmres : FooT {
    return FooT(a = 13)
}

def make_variant_with_subvariant_cmres : BarT {
    return BarT(foo = FooT(a = 13))
}

def FooT : FooT {
    return FooT(c = "1234")
}

def make_variant_with_field_call_to_cmres : BarT {
    return BarT(foo = FooT())
}

def make_variant_on_stack(which : bool) : FooT {
    var a = FooT(a = 1)
    var b = FooT(b = 2.)
    return which ? a : b
}

def make_variant_with_subvariant_on_stack(which : bool) : BarT {
    var a = BarT(foo = FooT(a = 1))
    var b = BarT(foo = FooT(b = 2.))
    return which ? a : b
}

[sideeffects]
def ask_variant_index {
    var f = FooT(b = 1.)
    return variant_index(f)
}

[sideeffects]
def change_variant_index {
    var f = FooT(b = 1.)
    unsafe {
        set_variant_index(f, 0)
    }
    return f as a
}

[test]
def test_variant(t : T?) {
    t |> run("variant field") @(t : T?) {
        var f : FooT
        t |> success(!jit_enabled() || is_jit_function(@@variant_field))
        var res = variant_field(f)
        t |> equal(res, int(0x3f800000))
    }
    t |> run("variant field move") @(t : T?) {
        var f : FooT
        t |> success(!jit_enabled() || is_jit_function(@@variant_field_move))
        f = FooT(a = 1234)
        var res = variant_field_move(f)
        t |> equal(0, f as a)
        t |> equal(1234, (res as foo) as a)
    }
    t |> run("variant is") @(t : T?) {
        var f : FooT
        t |> success(!jit_enabled() || is_jit_function(@@variant_is))
        t |> success(variant_is(f))
        t |> equal(1., f as b)
    }
    t |> run("variant as") @(t : T?) {
        var f : FooT
        t |> success(!jit_enabled() || is_jit_function(@@variant_as))
        t |> equal(2., variant_as(f))
        t |> equal(2., f as b)
    }
    t |> run("variant as panic") @(t : T?) {
        var f : FooT
        t |> success(!jit_enabled() || is_jit_function(@@variant_as_panic))
        var failed = false
        try {
            t |> equal(2, variant_as_panic(f))
        } recover {
            failed = true
        }
        t |> success(failed)
    }

    t |> run("make variant cmres") @(t : T?) {
        var f = make_variant_cmres()
        t |> success(!jit_enabled() || is_jit_function(@@make_variant_cmres))
        t |> success(f is a)
        t |> equal(13, f as a)
    }
    t |> run("make struct with substruct cmres") @(t : T?) {
        var f = make_variant_with_subvariant_cmres()
        t |> success(!jit_enabled() || is_jit_function(@@make_variant_with_subvariant_cmres))
        t |> success(f is foo)
        t |> success(f as foo is a)
        t |> equal(13, f as foo as a)
    }
    t |> run("make struct with substruct equal to call to cmres") @(t : T?) {
        var f = make_variant_with_field_call_to_cmres()
        t |> success(!jit_enabled() || is_jit_function(@@make_variant_with_field_call_to_cmres))
        t |> success(f is foo)
        t |> success(f as foo is c)
        t |> equal("1234", f as foo as c)
    }
    t |> run("make struct on stack") @(t : T?) {
        var a = make_variant_on_stack(true)
        var b = make_variant_on_stack(false)
        t |> success(!jit_enabled() || is_jit_function(@@make_variant_on_stack))
        t |> success(a is a)
        t |> equal(1, a as a)
        t |> success(b is b)
        t |> equal(2., b as b)
    }
    t |> run("make struct with substruct on stack") @(t : T?) {
        var a = make_variant_with_subvariant_on_stack(true)
        var b = make_variant_with_subvariant_on_stack(false)
        t |> success(!jit_enabled() || is_jit_function(@@make_variant_with_subvariant_on_stack))
        t |> success(a is foo)
        t |> success(a as foo is a)
        t |> equal(1, a as foo as a)
        t |> success(b is foo)
        t |> success(b as foo is b)
        t |> equal(2., b as foo as b)
    }
    t |> run("variant_index") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@ask_variant_index))
        t |> equal(1, ask_variant_index())
    }
    t |> run("set_variant_index") @(t : T?) {
        t |> success(!jit_enabled() || is_jit_function(@@change_variant_index))
        t |> equal(int(0x3f800000), change_variant_index())
    }
}

