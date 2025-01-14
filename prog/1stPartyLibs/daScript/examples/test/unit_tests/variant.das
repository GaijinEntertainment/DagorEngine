require UnitTest

struct Foo
    t   : variant<i_value:uint;f_value:float>?

typedef
    IorA = variant< i:int;a:array<int>;f:float;t:table<int;int> >

variant U_F             // this is syntax sugar for typedef U_F = variant<...>
    i_value : uint
    f_value : float

[sideeffects]
def test_ifs
    let i = makeSampleI()
    assert(i is i_value)
    assert(i as i_value == 1)
    let f = makeSampleF()
    assert(f is f_value)
    assert(f as f_value == 2.0)
    let s = makeSampleS()
    assert(s is s_value)
    assert(s as s_value == "3")

variant FooIS
    i : int
    s : string
    // j : int

[sideeffects]
def test_short_notation
    let e = [[FooIS 1]]
    assert(e is i)
    assert(e as i == 1)
    let ee = [[FooIS 1;"2"]]
    assert((ee[0] is i) && (ee[0] as i == 1))
    assert((ee[1] is s) && (ee[1] as s == "2"))
    return true

[export]
def test

    assert(typeinfo(sizeof type<U_F>)==8)

    var t : U_F
    assert(typeinfo(sizeof t)==8)
    unsafe
        t.f_value = 1.0                                                 // unsafe operation
        assert(t.i_value==0x3f800000)                                   // unsafe operation
    assert(variant_index(t)==0)

    assert(typeinfo(variant_index<i_value> t)==0)
    assert(typeinfo(variant_index<f_value> t)==1)

    assert(typeinfo(safe_variant_index<i_value> t)==0)
    assert(typeinfo(safe_variant_index<f_value> t)==1)
    assert(typeinfo(safe_variant_index<fg> t)==-1)
    assert(typeinfo(safe_variant_index<g> type<int>)==-1)

    assert(t is i_value)
    assert(t as i_value == 0x3f800000)
    unsafe
        set_variant_index(t, typeinfo(variant_index<f_value> t))        // unsafe operation
    assert(t as f_value == 1.0)

    assert(t ?as f_value ?? 2.0 == 1.0)
    assert(t ?as i_value ?? 1u == 1u)

    t as f_value = 2.0
    unsafe
        set_variant_index(t, typeinfo(variant_index<i_value> t))        // unsafe operation
    assert(t as i_value == 0x40000000)

    t as i_value = 0x3f800000
    unsafe
        set_variant_index(t, typeinfo(variant_index<f_value> t))        // unsafe operation
    assert(t as f_value == 1.0)

    var paniced = false
    try
        t as i_value = 0x40000000                                   // will assert, because t is f_value
    recover
        paniced = true
    assert(paniced)

    unsafe
        assert(t.f_value == 1.0)
    t = [[U_F i_value = 0x40000000]]
    assert(t as i_value == 0x40000000)
    t = [[U_F f_value = 1.0]]

    var t2 = [[U_F i_value=1u; f_value=2.0]]
    assert(length(t2)==2)
    assert(t2[0] is i_value && t2[0] as i_value==1u)
    assert(t2[1] is f_value && t2[1] as f_value==2.0)

    // with the latest changes ?. is now same as ?as
    /*
    var f : Foo
    var fp : Foo?
    unsafe
        assert ( fp?.t?.i_value ?? 13u == 13u )                         // unsafe ?.i_value
        fp = addr(f)
        assert ( fp?.t?.i_value ?? 14u == 14u )                         // unsafe ?.i_value
        f.t = addr(t)
        assert ( fp?.t?.i_value ?? 15u == 0x3f800000 )                  // unsafe ?.i_value
        assert ( fp?.t ?as i_value ?? 15u == 15u )
    */

    var x : IorA
    unsafe
        set_variant_index(x, 1)
        x.a <- [{for tt in range(10); tt}]
    var y := x
    assert(y is a)
    for tt in range(10)
        assert(y as a[tt]==tt)
    delete x
    assert(x is i)
    assert(y is a)
    for tt in range(10)
        assert(y as a[tt]==tt)

    test_ifs()

    test_short_notation()

    return true
