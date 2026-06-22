options gen2

require dastest/testing_boost public

// global var init promotion
var g_int8 : int8 = -100
var g_uint8 : uint8 = 200
var g_float : float = 42
var g_double : double = 100
var g_bitfield : bitfield = 0xFF
var g_int64 : int64 = 1
var g_uint64 : uint64 = 0xFFFFFFFF

struct Foo {
    a : float = 7
    b : uint8 = 255
    c : int64 = 1
    d : bitfield8 = 1
}

def returns_uint8() : uint8 {
    return 200
}

def returns_float() : float {
    return 42
}

[test]
def test_var_init_local(t : T?) {
    let i8 : int8 = -42
    let i16 : int16 = 30000
    let i64 : int64 = 1
    let u8 : uint8 = 250
    let u16 : uint16 = 60000
    let u : uint = 0xDEADBEEF
    let u64 : uint64 = 0xFFFFFFFF
    let f : float = 100
    let d : double = 100
    let bf8 : bitfield8 = 255
    let bf16 : bitfield16 = 65535
    let bf : bitfield = 0xFF
    let bf64 : bitfield64 = 0xFF
    t |> equal(int(i8), -42, "int8 promotion")
    t |> equal(int(i16), 30000, "int16 promotion")
    t |> equal(int(i64), 1, "int64 promotion")
    t |> equal(int(u8), 250, "uint8 promotion")
    t |> equal(int(u16), 60000, "uint16 promotion")
    t |> equal(u, 0xDEADBEEFu, "uint promotion")
    t |> equal(uint(u64), 0xFFFFFFFFu, "uint64 promotion")
    t |> equal(f, 100.0f, "float promotion")
    t |> equal(d, 100.0lf, "double promotion")
    t |> equal(int(bf8), 255, "bitfield8 promotion")
    t |> equal(int(bf16), 65535, "bitfield16 promotion")
    t |> equal(int(bf), 255, "bitfield promotion")
    t |> equal(int(bf64), 255, "bitfield64 promotion")
}

[test]
def test_var_init_global(t : T?) {
    t |> equal(int(g_int8), -100, "global int8 init")
    t |> equal(int(g_uint8), 200, "global uint8 init")
    t |> equal(g_float, 42.0f, "global float init")
    t |> equal(g_double, 100.0lf, "global double init")
    t |> equal(int(g_bitfield), 255, "global bitfield init")
    t |> equal(int(g_int64), 1, "global int64 init")
    t |> equal(uint(g_uint64), 0xFFFFFFFFu, "global uint64 init")
}

[test]
def test_struct_field_decl_init(t : T?) {
    let f = Foo()
    t |> equal(f.a, 7.0f, "struct float field init")
    t |> equal(int(f.b), 255, "struct uint8 field init")
    t |> equal(int(f.c), 1, "struct int64 field init")
    t |> equal(int(f.d), 1, "struct bitfield8 field init")
}

[test]
def test_struct_ctor_init(t : T?) {
    let f = Foo(a = 3, b = 100, c = 2)
    t |> equal(f.a, 3.0f, "ctor float promotion")
    t |> equal(int(f.b), 100, "ctor uint8 promotion")
    t |> equal(int(f.c), 2, "ctor int64 promotion")
}

[test]
def test_copy_clone_compound(t : T?) {
    var f : float = 0      // nolint:LINT010 — init exists so type is fixed; first test rewrites it
    f = 42
    t |> equal(f, 42.0f, "copy promotion")
    f := 100
    t |> equal(f, 100.0f, "clone promotion")
    f += 2
    t |> equal(f, 102.0f, "compound assignment promotion")
    f -= 2
    t |> equal(f, 100.0f, "compound subtract promotion")
    f *= 2
    t |> equal(f, 200.0f, "compound mul promotion")
    f /= 2
    t |> equal(f, 100.0f, "compound div promotion")
}

[test]
def test_op2_arith(t : T?) {
    let a : float = 10
    t |> equal(a + 1, 11.0f, "right-side int promotion in +")
    t |> equal(1 + a, 11.0f, "left-side int promotion in +")
    t |> equal(a - 1, 9.0f, "right-side int promotion in -")
    t |> equal(a * 2, 20.0f, "right-side int promotion in *")
    t |> equal(a / 2, 5.0f, "right-side int promotion in /")
}

[test]
def test_op2_compare(t : T?) {
    let a : float = 10
    t |> equal(a < 11, true, "compare with int literal")
    t |> equal(a > 5, true, "compare with int literal (gt)")
    t |> equal(a == 10, true, "equality with int literal")
}

[test]
def test_return_promotion(t : T?) {
    t |> equal(int(returns_uint8()), 200, "return uint8 promotion")
    t |> equal(returns_float(), 42.0f, "return float promotion")
}

[test]
def test_array_element_promotion(t : T?) {
    // first element fixes the element type, subsequent int literals
    // flow through visitMakeArrayIndex's promotion path
    let f <- [1.0f, 2, 3, 16777216]
    t |> equal(f[0], 1.0f, "array elem 0 (float literal)")
    t |> equal(f[1], 2.0f, "array elem 1 (int promoted)")
    t |> equal(f[2], 3.0f, "array elem 2 (int promoted)")
    t |> equal(f[3], 16777216.0f, "array elem 3 (int promoted at 2^24)")
    let u <- [uint8(10), 0, 200, 255]
    t |> equal(int(u[0]), 10, "uint8 array elem 0")
    t |> equal(int(u[1]), 0, "uint8 array elem 1 (in-range int promoted)")
    t |> equal(int(u[2]), 200, "uint8 array elem 2 (in-range int promoted)")
    t |> equal(int(u[3]), 255, "uint8 array elem 3 (boundary int promoted)")
}

[test]
def test_boundary_values(t : T?) {
    // exact boundary values that fit
    let i8_min : int8 = -128
    let i8_max : int8 = 127
    let u8_max : uint8 = 255
    let i16_min : int16 = -32768
    let i16_max : int16 = 32767
    let u16_max : uint16 = 65535
    let f_exact : float = 16777216    // 2^24 — exactly representable
    t |> equal(int(i8_min), -128, "int8 min boundary")
    t |> equal(int(i8_max), 127, "int8 max boundary")
    t |> equal(int(u8_max), 255, "uint8 max boundary")
    t |> equal(int(i16_min), -32768, "int16 min boundary")
    t |> equal(int(i16_max), 32767, "int16 max boundary")
    t |> equal(int(u16_max), 65535, "uint16 max boundary")
    t |> equal(f_exact, 16777216.0f, "float exact 2^24")
}
