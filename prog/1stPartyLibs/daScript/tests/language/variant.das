options gen2
require UnitTest
require dastest/testing_boost public

struct Foo {
    t   : variant<i_value : uint; f_value : float>?
}

typedef IorA = variant< i : int; a : array<int>; f : float; t : table<int; int> >


variant U_F {             // this is syntax sugar for typedef U_F = variant<...>
    i_value : uint
    f_value : float
}

[sideeffects]
def test_ifs(t : T?) {
    let i = makeSampleI()
    t |> success(i is i_value)
    t |> success(i as i_value == 1)
    let f = makeSampleF()
    t |> success(f is f_value)
    t |> success(f as f_value == 2.0)
    let s = makeSampleS()
    t |> success(s is s_value)
    t |> equal(s as s_value, "3")
}

variant FooIS {
    i : int
    s : string
    // j : int
}

[sideeffects]
def test_short_notation(t : T?) {
    let e = FooIS(i = 1)
    t |> success(e is i)
    t |> success(e as i == 1)
    let ee = fixed_array<FooIS>(1, "2")
    t |> success((ee[0] is i) && (ee[0] as i == 1))
    t |> success((ee[1] is s) && (ee[1] as s == "2"))
}

[test]
def test_variant(t : T?) {

    t |> equal(typeinfo sizeof(type<U_F>), 8)

    var u : U_F
    t |> equal(typeinfo sizeof(u), 8)
    unsafe {
        u.f_value = 1.0                                                 // unsafe operation
        t |> equal(u.i_value, 0x3f800000)                               // unsafe operation
    }
    t |> equal(variant_index(u), 0)

    t |> equal(typeinfo variant_index<i_value>(u), 0)
    t |> equal(typeinfo variant_index<f_value>(u), 1)

    t |> equal(typeinfo safe_variant_index < i_value > (u), 0)
    t |> equal(typeinfo safe_variant_index < f_value > (u), 1)
    t |> equal(typeinfo safe_variant_index < fg > (u), -1)
    t |> equal(typeinfo safe_variant_index < g > (type<int>), -1)

    t |> success(u is i_value)
    t |> equal(u as i_value, 0x3f800000)
    unsafe {
        set_variant_index(u, typeinfo variant_index<f_value>(u))        // unsafe operation
    }
    t |> equal(u as f_value, 1.0)

    t |> equal(u ?as f_value ?? 2.0, 1.0)
    t |> equal(u ?as i_value ?? 1u, 1u)

    u as f_value = 2.0
    unsafe {
        set_variant_index(u, typeinfo variant_index<i_value>(u))        // unsafe operation
    }
    t |> equal(u as i_value, 0x40000000)

    u as i_value = 0x3f800000
    unsafe {
        set_variant_index(u, typeinfo variant_index<f_value>(u))        // unsafe operation
    }
    t |> equal(u as f_value, 1.0)

    var paniced = false
    try {
        u as i_value = 0x40000000                                   // will assert, because u is f_value
    } recover {
        paniced = true
    }
    t |> success(paniced)

    unsafe {
        t |> equal(u.f_value, 1.0)
    }
    u = U_F(i_value = 0x40000000)
    t |> equal(u as i_value, 0x40000000)
    u = U_F(f_value = 1.0)

    var t2 = [U_F(i_value=1u), U_F(f_value=2.0)]
    t |> equal(length(t2), 2)
    t |> success(t2[0] is i_value && t2[0] as i_value == 1u)
    t |> success(t2[1] is f_value && t2[1] as f_value == 2.0)

    // with the latest changes ?. is now same as ?as
    /*
    var f : Foo
    var fp : Foo?
    unsafe
        assert ( fp?.u?.i_value ?? 13u == 13u )                         // unsafe ?.i_value
        fp = addr(f)
        assert ( fp?.u?.i_value ?? 14u == 14u )                         // unsafe ?.i_value
        f.u = addr(u)
        assert ( fp?.u?.i_value ?? 15u == 0x3f800000 )                  // unsafe ?.i_value
        assert ( fp?.u ?as i_value ?? 15u == 15u )
    */

    var x : IorA
    unsafe {
        set_variant_index(x, 1)
        x.a <- [for (tt in range(10)); tt]
    }
    var y := x
    t |> success(y is a)
    for (tt in range(10)) {
        t |> equal(y as a[tt], tt)
    }
    delete x
    t |> success(x is i)
    t |> success(y is a)
    for (tt in range(10)) {
        t |> equal(y as a[tt], tt)
    }

    test_ifs(t)

    test_short_notation(t)
}
