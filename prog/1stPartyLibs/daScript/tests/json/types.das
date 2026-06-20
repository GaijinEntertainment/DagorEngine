options gen2
require dastest/testing_boost public
require daslib/json_boost

def test_equ(t : T?; x : auto(TT)) {
    var jv = JV(x)
    // to_log(LOG_INFO, "{x}:{typeinfo(typename x)} serialized as {write_json(jv)}\n")
    // Generic driver: TT can be scalar or struct/table — keep from_JV uniform.
    t |> equal(x, from_JV(jv, decltype_noref(x)))  // nolint:STYLE020
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

def test_array_form(t : T?; jsonText : string; want : auto(VecT)) {
    var err : string
    var v = read_json(jsonText, err)
    t |> equal(empty(err), true, "read_json '{jsonText}' parses: {err}")
    t |> equal(from_JV(v, type<VecT>, default<VecT>), want, "from_JV<vec> '{jsonText}'")
    unsafe {
        delete v
    }
}

[test]
def vector_array_form(t : T?) {
    //! from_JV<vecN> accepts the JSON array form '[x,y,z,w]' (what sprint_json /
    //! debug_json_value emit) in addition to the object form '{x,y,z,w}' (what
    //! JV(vec) emits). Both shapes round-trip to the same vector value.
    test_array_form(t, "[1, 2]", float2(1, 2))
    test_array_form(t, "[1, 2, 3]", float3(1, 2, 3))
    test_array_form(t, "[1, 2, 3, 4]", float4(1, 2, 3, 4))
    test_array_form(t, "[1, 2]", int2(1, 2))
    test_array_form(t, "[1, 2, 3]", int3(1, 2, 3))
    test_array_form(t, "[1, 2, 3, 4]", int4(1, 2, 3, 4))
    test_array_form(t, "[1, 2]", uint2(1, 2))
    test_array_form(t, "[1, 2, 3]", uint3(1, 2, 3))
    test_array_form(t, "[1, 2, 3, 4]", uint4(1, 2, 3, 4))
    // Short / empty arrays fall back to default field-by-field.
    test_array_form(t, "[]", float3(0, 0, 0))
    test_array_form(t, "[7]", float3(7, 0, 0))
    test_array_form(t, "[7, 8]", float3(7, 8, 0))
    // Object form still works after the array-form branch.
    var err : string
    var obj = read_json("\{\"x\":1, \"y\":2, \"z\":3\}", err)
    t |> equal(from_JV(obj, type<float3>, default<float3>), float3(1, 2, 3),
               "object form still accepted")
    unsafe {
        delete obj
    }
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
        return false if (A != B)
    }
    return true
}

def operator ==(a, b : SFoo[2] | #) {
    for (A, B in a, b) {
        return false if (!(A == B))
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
        return false if (A != B)
    }
    return true
}

def operator ==(a, b : array<SFoo>) {
    for (A, B in a, b) {
        return false if (!(A == B))
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
    return false if (a |> length != b |> length)
    for (k, v in keys(a), values(a)) {
        return false if (!(b |> key_exists(k)))
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
    return false if (a.c |> length != b.c |> length)
    for (ac, bc in a.c, b.c) {
        return false if (ac != bc)
    }
    return a.a == b.a && a.b == b.b
}

[test]
def moveable_struct_types(t : T?) {
    test_equ(t, MFoo(a = 1, b = 2., c <- array<int>(1, 2, 3)))
}
