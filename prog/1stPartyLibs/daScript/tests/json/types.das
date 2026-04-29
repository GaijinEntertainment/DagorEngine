options gen2
require dastest/testing_boost public
require daslib/json_boost

def test_equ(t : T?; x : auto(TT)) {
    var jv = JV(x)
    // to_log(LOG_INFO, "{x}:{typeinfo(typename x)} serialized as {write_json(jv)}\n")
    t |> equal(x, from_JV(jv, decltype_noref(x)))
    unsafe {
        delete jv
    }
}

[test]
def basic_types(t : T?) {
    test_equ(t, 1)
    test_equ(t, 2u)
    test_equ(t, int8(1))
    test_equ(t, uint8(2))
    test_equ(t, int16(1))
    test_equ(t, uint16(2))
    test_equ(t, 1234567890123456789l)
    test_equ(t, 0x1234567812345678ul)
    test_equ(t, 1.)
    test_equ(t, 2.lf)
    test_equ(t, bitfield(13))
}

[test]
def vector_types(t : T?) {
    test_equ(t, float2(1, 2))
    test_equ(t, float3(1, 2, 3))
    test_equ(t, float4(1, 2, 3, 4))
    test_equ(t, int2(1, 2))
    test_equ(t, int3(1, 2, 3))
    test_equ(t, int4(1, 2, 3, 4))
    test_equ(t, uint2(1, 2))
    test_equ(t, uint3(1, 2, 3))
    test_equ(t, uint4(1, 2, 3, 4))
}

enum FooBar {
    foo
    bar
}

[test]
def enum_types(t : T?) {
    test_equ(t, FooBar.foo)
    test_equ(t, FooBar.bar)
}

struct SFoo {
    a : int
    b : float
}

def operator ==(a, b : SFoo | #) {
    return a.a == b.a && a.b == b.b
}

struct NFoo {
    c : double
    s : SFoo
}

def operator ==(a, b : NFoo) {
    return a.c == b.c && a.s == b.s
}

[test]
def struct_types(t : T?) {
    test_equ(t, SFoo(a = 1, b = 2.))
    test_equ(t, NFoo(c = 1.lf, s = SFoo(a = 1, b = 2.)))
}

def operator ==(a, b : int[4]) {
    for (A, B in a, b) {
        if (A != B) {
            return false
        }
    }
    return true
}

def operator ==(a, b : SFoo[2] | #) {
    for (A, B in a, b) {
        if (!(A == B)) {
            return false
        }
    }
    return true
}

[test]
def dim_types(t : T?) {
    test_equ(t, fixed_array<int>(1, 2, 3, 4))
    test_equ(t, [SFoo(a=1, b=2.), SFoo(a=2, b=3.)])
}

def operator ==(a, b : array<int>) {
    for (A, B in a, b) {
        if (A != B) {
            return false
        }
    }
    return true
}

def operator ==(a, b : array<SFoo>) {
    for (A, B in a, b) {
        if (!(A == B)) {
            return false
        }
    }
    return true
}

[test]
def array_types(t : T?) {
    test_equ(t, array<int>(1, 2, 3, 4))
    test_equ(t, [SFoo(a=1, b=2.), SFoo(a=2, b=3.)])
}

typedef TFoo = tuple<int; float; string>

def operator ==(a, b : TFoo | #) {
    return a._0 == b._0 && a._1 == b._1 && a._2 == b._2
}

[test]
def tuple_types(t : T?) {
    test_equ(t, (1, 2.0, "3"))
}

def op_equ_tab(a, b : table<string; auto(TT)>) {
    if (a |> length != b |> length) {
        return false
    }
    for (k, v in keys(a), values(a)) {
        if (!(b |> key_exists(k))) {
            return false
        }
        var same = false
        b |> get(k) $(pv) {
            same = pv == v
        }
    }
    return true
}

def operator ==(a, b : table<string; int>) {
    return op_equ_tab(a, b)
}

def operator ==(a, b : table<string; SFoo>) {
    return op_equ_tab(a, b)
}

[test]
def string_table(t : T?) {
    test_equ(t, {"one" => 1, "two" => 2})
    test_equ(t, {"tab1" => SFoo(a = 1, b = 2.), "tab2" => SFoo(a = 2, b = 3.)})
}

variant VFoo {
    i : int
    f : float
}

def operator ==(a, b : VFoo) {
    if ((a is i) && (b is i)) {
        return (a as i) == (b as i)
    } elif ((a is f) && (b is f)) {
        return (a as f) == (b as f)
    } else {
        return false
    }
}

[test]
def variant_types(t : T?) {
    test_equ(t, VFoo(i = 1))
    test_equ(t, VFoo(f = 1.))
}

struct MFoo {
    a : int
    b : float
    c : array<int>
}

def operator ==(a, b : MFoo) {
    if (a.c |> length != b.c |> length) {
        return false
    }
    for (ac, bc in a.c, b.c) {
        if (ac != bc) {
            return false
        }
    }
    return a.a == b.a && a.b == b.b
}

[test]
def moveable_struct_types(t : T?) {
    test_equ(t, MFoo(a = 1, b = 2., c <- array<int>(1, 2, 3)))
}
