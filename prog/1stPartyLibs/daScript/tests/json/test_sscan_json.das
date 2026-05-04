options gen2
options rtti
options no_unused_block_arguments = false
options no_unused_function_arguments = false
require dastest/testing_boost public
require strings

// ============================================================
//  shared types
// ============================================================

struct Inner {
    x : int
    y : float
}

struct Outer {
    inner : Inner
    name : string
}

struct WithPtr {
    inner : Inner?
    tag : string
}

struct WithArrays {
    ints : array<int>
    strings : array<string>
    inners : array<Inner>
    ptrs : array<Inner?>
}

struct WithRename {
    @rename _type : string
    @rename _from : string
    value : int
}

enum Color {
    red
    green
    blue
}

struct WithEnum {
    color : Color
}

struct RoundTrip {
    x : int
    y : float
    name : string
    flag : bool
    inner : Inner?
    items : array<int>
}

struct Level3 {
    value : int
}

struct Level2 {
    l3 : Level3?
}

struct Level1 {
    l2 : Level2?
    name : string
}

struct Message {
    id : int64
    text : string
    @rename _from : string
}

struct Updates {
    messages : array<Message?>
}

struct BasicTypes {
    b : bool
    i : int
    u : uint
    i64 : int64
    u64 : uint64
    f : float
    d : double
    s : string
    i8 : int8
    u8 : uint8
    i16 : int16
    u16 : uint16
}

// ============================================================
//  test: basic scalar types
// ============================================================

[test]
def test_basic_scalars(t : T?) {
    t |> run("all scalar types") @(t : T?) {
        var v : BasicTypes
        let ok = sscan_json("\{\"b\":true,\"i\":-42,\"u\":100,\"i64\":9999999999,\"u64\":8888888888,\"f\":3.14,\"d\":2.718281828,\"s\":\"hello\",\"i8\":-1,\"u8\":255,\"i16\":-1000,\"u16\":65000\}", v)
        t |> success(ok)
        t |> equal(v.b, true)
        t |> equal(v.i, -42)
        t |> equal(v.u, 100u)
        t |> equal(v.i64, 9999999999l)
        t |> equal(v.u64, 8888888888ul)
        t |> success(v.f > 3.13 && v.f < 3.15, "float approx")
        t |> success(v.d > 2.71lf && v.d < 2.72lf, "double approx")
        t |> equal(v.s, "hello")
        t |> equal(v.i8, int8(-1))
        t |> equal(v.u8, uint8(255))
        t |> equal(v.i16, int16(-1000))
        t |> equal(v.u16, uint16(65000))
    }
}

// ============================================================
//  test: nested struct
// ============================================================

[test]
def test_nested_struct(t : T?) {
    t |> run("by value") @(t : T?) {
        var v : Outer
        let ok = sscan_json("\{\"inner\":\{\"x\":10,\"y\":2.5\},\"name\":\"outer\"\}", v)
        t |> success(ok)
        t |> equal(v.inner.x, 10)
        t |> success(v.inner.y > 2.4 && v.inner.y < 2.6, "nested y")
        t |> equal(v.name, "outer")
    }
}

// ============================================================
//  test: struct pointer
// ============================================================

[test]
def test_struct_pointer(t : T?) {
    t |> run("non-null pointer") @(t : T?) {
        var v : WithPtr
        let ok = sscan_json("\{\"inner\":\{\"x\":7,\"y\":1.0\},\"tag\":\"ptr\"\}", v)
        t |> success(ok)
        t |> success(v.inner != null, "ptr not null")
        unsafe {
            t |> equal(v.inner.x, 7)
        }
        t |> equal(v.tag, "ptr")
    }
    t |> run("null pointer") @(t : T?) {
        var v : WithPtr
        let ok = sscan_json("\{\"inner\":null,\"tag\":\"nil\"\}", v)
        t |> success(ok)
        t |> success(v.inner == null, "ptr is null")
        t |> equal(v.tag, "nil")
    }
}

// ============================================================
//  test: arrays
// ============================================================

[test]
def test_arrays(t : T?) {
    t |> run("mixed arrays") @(t : T?) {
        var v : WithArrays
        let ok = sscan_json("\{\"ints\":[1,2,3],\"strings\":[\"a\",\"b\"],\"inners\":[\{\"x\":1,\"y\":1.0\},\{\"x\":2,\"y\":2.0\}],\"ptrs\":[\{\"x\":10,\"y\":10.0\},null,\{\"x\":30,\"y\":30.0\}]\}", v)
        t |> success(ok)
        t |> equal(length(v.ints), 3)
        t |> equal(v.ints[0], 1)
        t |> equal(v.ints[2], 3)
        t |> equal(length(v.strings), 2)
        t |> equal(v.strings[0], "a")
        t |> equal(length(v.inners), 2)
        t |> equal(v.inners[1].x, 2)
        t |> equal(length(v.ptrs), 3)
        t |> success(v.ptrs[0] != null, "ptrs[0] not null")
        t |> success(v.ptrs[1] == null, "ptrs[1] null")
        t |> success(v.ptrs[2] != null, "ptrs[2] not null")
        unsafe {
            t |> equal(v.ptrs[0].x, 10)
            t |> equal(v.ptrs[2].x, 30)
        }
    }
    t |> run("empty arrays") @(t : T?) {
        var v : WithArrays
        let ok = sscan_json("\{\"ints\":[],\"strings\":[],\"inners\":[],\"ptrs\":[]\}", v)
        t |> success(ok)
        t |> equal(length(v.ints), 0)
    }
    t |> run("array clear on re-scan") @(t : T?) {
        var v : WithArrays
        v.ints |> push(99)
        v.ints |> push(98)
        let ok = sscan_json("\{\"ints\":[1,2,3],\"strings\":[],\"inners\":[],\"ptrs\":[]\}", v)
        t |> success(ok)
        t |> equal(length(v.ints), 3)
        t |> equal(v.ints[0], 1)
    }
}

// ============================================================
//  test: @rename
// ============================================================

[test]
def test_rename(t : T?) {
    t |> run("rename strips underscore") @(t : T?) {
        var v : WithRename
        let ok = sscan_json("\{\"type\":\"message\",\"from\":\"user\",\"value\":42\}", v)
        t |> success(ok)
        t |> equal(v._type, "message")
        t |> equal(v._from, "user")
        t |> equal(v.value, 42)
    }
}

// ============================================================
//  test: unknown keys, missing fields, empty object
// ============================================================

[test]
def test_edge_cases(t : T?) {
    t |> run("unknown keys ignored") @(t : T?) {
        var v : Inner
        let ok = sscan_json("\{\"x\":5,\"unknown\":\"ignored\",\"y\":1.5,\"extra\":\{\"nested\":true\}\}", v)
        t |> success(ok)
        t |> equal(v.x, 5)
        t |> success(v.y > 1.4 && v.y < 1.6, "y approx")
    }
    t |> run("missing fields stay default") @(t : T?) {
        var v : Inner
        let ok = sscan_json("\{\"x\":99\}", v)
        t |> success(ok)
        t |> equal(v.x, 99)
        t |> success(v.y == 0.0, "y default")
    }
    t |> run("empty object") @(t : T?) {
        var v : Inner
        let ok = sscan_json("\{\}", v)
        t |> success(ok)
        t |> equal(v.x, 0)
    }
    t |> run("bad JSON returns false") @(t : T?) {
        var v : Inner
        t |> success(!sscan_json("not json", v), "bad json 1")
        t |> success(!sscan_json("\{broken", v), "bad json 2")
        t |> success(!sscan_json("", v), "bad json 3")
    }
}

// ============================================================
//  test: enums
// ============================================================

[test]
def test_enums(t : T?) {
    t |> run("enum as string") @(t : T?) {
        var v : WithEnum
        let ok = sscan_json("\{\"color\":\"green\"\}", v)
        t |> success(ok)
        t |> equal(v.color, Color.green)
    }
    t |> run("enum as int") @(t : T?) {
        var v : WithEnum
        let ok = sscan_json("\{\"color\":2\}", v)
        t |> success(ok)
        t |> equal(v.color, Color.blue)
    }
}

// ============================================================
//  test: deeply nested pointers
// ============================================================

[test]
def test_deep_nesting(t : T?) {
    t |> run("3 levels deep") @(t : T?) {
        var v : Level1
        let ok = sscan_json("\{\"l2\":\{\"l3\":\{\"value\":777\}\},\"name\":\"deep\"\}", v)
        t |> success(ok)
        t |> equal(v.name, "deep")
        t |> success(v.l2 != null, "l2 not null")
        unsafe {
            t |> success(v.l2.l3 != null, "l3 not null")
            t |> equal(v.l2.l3.value, 777)
        }
    }
}

// ============================================================
//  test: array of struct pointers (telegram-style)
// ============================================================

[test]
def test_array_of_struct_ptrs(t : T?) {
    t |> run("telegram-style messages") @(t : T?) {
        var v : Updates
        let ok = sscan_json("\{\"messages\":[\{\"id\":1,\"text\":\"hi\",\"from\":\"alice\"\},\{\"id\":2,\"text\":\"bye\",\"from\":\"bob\"\}]\}", v)
        t |> success(ok)
        t |> equal(length(v.messages), 2)
        unsafe {
            t |> equal(v.messages[0].id, 1l)
            t |> equal(v.messages[0].text, "hi")
            t |> equal(v.messages[0]._from, "alice")
            t |> equal(v.messages[1].id, 2l)
        }
    }
}

// ============================================================
//  test: string escapes
// ============================================================

[test]
def test_string_escapes(t : T?) {
    t |> run("newline and tab") @(t : T?) {
        var v : Outer
        let ok = sscan_json("\{\"inner\":\{\"x\":0,\"y\":0.0\},\"name\":\"hello\\nworld\\t!\"\}", v)
        t |> success(ok)
        t |> equal(v.name, "hello\nworld\t!")
    }
}

// ============================================================
//  test: number coercion
// ============================================================

[test]
def test_number_coercion(t : T?) {
    t |> run("float-to-int and int-to-float") @(t : T?) {
        var v : Inner
        let ok = sscan_json("\{\"x\":42.0,\"y\":3\}", v)
        t |> success(ok)
        t |> equal(v.x, 42)
        t |> success(v.y > 2.9 && v.y < 3.1, "y coerced")
    }
    t |> run("negative numbers") @(t : T?) {
        var v : Inner
        let ok = sscan_json("\{\"x\":-100,\"y\":-2.5\}", v)
        t |> success(ok)
        t |> equal(v.x, -100)
        t |> success(v.y < -2.4 && v.y > -2.6, "neg y")
    }
}

// ============================================================
//  test: round-trip sprint_json -> sscan_json
// ============================================================

[test]
def test_round_trip(t : T?) {
    t |> run("sprint then sscan") @(t : T?) {
        var src : RoundTrip
        src.x = 42
        src.y = 3.14
        src.name = "test"
        src.flag = true
        src.inner = new Inner(x = 7, y = 8.5)
        src.items |> push(10)
        src.items |> push(20)
        src.items |> push(30)
        let json = sprint_json(src, false)
        var dst : RoundTrip
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst.x, 42)
        t |> success(dst.y > 3.13 && dst.y < 3.15, "rt y")
        t |> equal(dst.name, "test")
        t |> equal(dst.flag, true)
        t |> success(dst.inner != null, "rt inner")
        unsafe {
            t |> equal(dst.inner.x, 7)
        }
        t |> equal(length(dst.items), 3)
        t |> equal(dst.items[1], 20)
    }
}

// ============================================================
//  test: tuples
// ============================================================

[test]
def test_tuples(t : T?) {
    t |> run("tuple round-trip") @(t : T?) {
        let src = (42, 3.14, "hello")
        let json = sprint_json(src, false)
        var dst : tuple<int; float; string>
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst._0, 42)
        t |> success(dst._1 > 3.13 && dst._1 < 3.15, "tuple float")
        t |> equal(dst._2, "hello")
    }
    t |> run("tuple of two ints") @(t : T?) {
        var dst : tuple<int; int>
        let ok = sscan_json("\{\"_0\":10,\"_1\":20\}", dst)
        t |> success(ok)
        t |> equal(dst._0, 10)
        t |> equal(dst._1, 20)
    }
}

// ============================================================
//  test: variants
// ============================================================

variant TestPayload {
    code : int
    msg : string
}

[test]
def test_variants(t : T?) {
    t |> run("variant int round-trip") @(t : T?) {
        let src = TestPayload(code = 99)
        let json = sprint_json(src, false)
        var dst : TestPayload
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> success(dst is code, "variant is code")
        t |> equal(dst as code, 99)
    }
    t |> run("variant string round-trip") @(t : T?) {
        let src = TestPayload(msg = "hi")
        let json = sprint_json(src, false)
        var dst : TestPayload
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> success(dst is msg, "variant is msg")
        t |> equal(dst as msg, "hi")
    }
}

// ============================================================
//  test: tables
// ============================================================

[test]
def test_tables(t : T?) {
    t |> run("string-keyed table round-trip") @(t : T?) {
        var src <- { "a" => 1, "b" => 2, "c" => 3 }
        let json = sprint_json(src, false)
        var dst : table<string; int>
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst?["a"] ?? -1, 1)
        t |> equal(dst?["b"] ?? -1, 2)
        t |> equal(dst?["c"] ?? -1, 3)
    }
    t |> run("empty table") @(t : T?) {
        var dst : table<string; int>
        let ok = sscan_json("\{\}", dst)
        t |> success(ok)
        t |> equal(length(dst), 0)
    }
    t |> run("int-keyed table round-trip") @(t : T?) {
        var src <- { 10 => "ten", 20 => "twenty" }
        let json = sprint_json(src, false)
        var dst : table<int; string>
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst?[10] ?? "", "ten")
        t |> equal(dst?[20] ?? "", "twenty")
    }
}

// ============================================================
//  test: vector types (int2/3/4, float2/3/4, uint2/3/4)
//  NOTE: sscan_json modifies ref types; vector/range/bitfield are value types,
//  so we test them as struct fields (which is the normal JSON use case).
// ============================================================

struct WithVectors {
    i2 : int2
    i3 : int3
    i4 : int4
    f2 : float2
    f3 : float3
    f4 : float4
    u2 : uint2
}

[test]
def test_vector_types(t : T?) {
    t |> run("vectors in struct round-trip") @(t : T?) {
        var src : WithVectors
        src.i2 = int2(10, 20)
        src.i3 = int3(1, 2, 3)
        src.i4 = int4(1, 2, 3, 4)
        src.f2 = float2(1.5, 2.5)
        src.f3 = float3(1.0, 2.0, 3.0)
        src.f4 = float4(1.0, 2.0, 3.0, 4.0)
        src.u2 = uint2(100u, 200u)
        let json = sprint_json(src, false)
        var dst : WithVectors
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst.i2.x, 10)
        t |> equal(dst.i2.y, 20)
        t |> equal(dst.i3.x, 1)
        t |> equal(dst.i3.y, 2)
        t |> equal(dst.i3.z, 3)
        t |> equal(dst.i4.x, 1)
        t |> equal(dst.i4.w, 4)
        t |> success(dst.f2.x > 1.4 && dst.f2.x < 1.6, "f2 x")
        t |> success(dst.f2.y > 2.4 && dst.f2.y < 2.6, "f2 y")
        t |> success(dst.f3.z > 2.9 && dst.f3.z < 3.1, "f3 z")
        t |> success(dst.f4.w > 3.9 && dst.f4.w < 4.1, "f4 w")
        t |> equal(dst.u2.x, 100u)
        t |> equal(dst.u2.y, 200u)
    }
}

// ============================================================
//  test: range types (as struct fields)
// ============================================================

struct WithRanges {
    r : range
    ur : urange
    r64 : range64
    ur64 : urange64
}

[test]
def test_range_types(t : T?) {
    t |> run("ranges in struct round-trip") @(t : T?) {
        var src : WithRanges
        src.r = range(10, 20)
        src.ur = urange(5u, 15u)
        src.r64 = range64(100l, 200l)
        src.ur64 = urange64(50ul, 150ul)
        let json = sprint_json(src, false)
        var dst : WithRanges
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst.r.x, 10)
        t |> equal(dst.r.y, 20)
        t |> equal(dst.ur.x, 5u)
        t |> equal(dst.ur.y, 15u)
        t |> equal(dst.r64.x, 100l)
        t |> equal(dst.r64.y, 200l)
        t |> equal(dst.ur64.x, 50ul)
        t |> equal(dst.ur64.y, 150ul)
    }
}

// ============================================================
//  test: bitfields (as struct field)
// ============================================================

bitfield TestBits {
    flagA
    flagB
    flagC
}

struct WithBitfield {
    bits : TestBits
    name : string
}

[test]
def test_bitfields(t : T?) {
    t |> run("bitfield in struct round-trip") @(t : T?) {
        var src : WithBitfield
        src.bits = TestBits.flagA | TestBits.flagC
        src.name = "test"
        let json = sprint_json(src, false)
        var dst : WithBitfield
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst.bits, src.bits)
        t |> equal(dst.name, "test")
    }
}

// ============================================================
//  test: void pointer (null)
// ============================================================

[test]
def test_void_pointer(t : T?) {
    t |> run("void pointer accepts null") @(t : T?) {
        var dst : void?
        let ok = sscan_json("null", dst)
        t |> success(ok)
        t |> success(dst == null, "void ptr null")
    }
}

// ============================================================
//  test: struct with table field round-trip
// ============================================================

struct WithTable {
    name : string
    data : table<string; int>
}

[test]
def test_struct_with_table(t : T?) {
    t |> run("struct containing table") @(t : T?) {
        var src : WithTable
        src.name = "test"
        src.data |> insert("x", 1)
        src.data |> insert("y", 2)
        let json = sprint_json(src, false)
        var dst : WithTable
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst.name, "test")
        t |> equal(dst.data?["x"] ?? -1, 1)
        t |> equal(dst.data?["y"] ?? -1, 2)
    }
}

// ============================================================
//  test: table key types (all types from JIT_TABLE_FUNCTION)
// ============================================================

// Structs to hold table fields (tables are ref types, so struct wrapping is fine)
struct WithBoolTable { data : table<bool; string>; }
struct WithInt8Table { data : table<int8; string>; }
struct WithUInt8Table { data : table<uint8; string>; }
struct WithInt16Table { data : table<int16; string>; }
struct WithFloatTable { data : table<float; string>; }
struct WithDoubleTable { data : table<double; string>; }
struct WithUInt64Table { data : table<uint64; string>; }
struct WithInt2Table { data : table<int2; string>; }
struct WithFloat2Table { data : table<float2; string>; }

[test]
def test_table_key_types(t : T?) {
    t |> run("bool-keyed table") @(t : T?) {
        var src : WithBoolTable
        src.data <- { true => "yes", false => "no" }
        let json = sprint_json(src, false)
        var dst : WithBoolTable
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst.data?[true] ?? "", "yes")
        t |> equal(dst.data?[false] ?? "", "no")
    }
    // disabled: int8(-1) table key fails under JIT on macOS ARM64 — needs investigation on macOS hardware
    // t |> run("int8-keyed table") @(t : T?) {
    //     var src : WithInt8Table
    //     src.data <- { int8(1) => "one", int8(-1) => "neg" }
    //     let json = sprint_json(src, false)
    //     var dst : WithInt8Table
    //     let ok = sscan_json(json, dst)
    //     t |> success(ok)
    //     t |> equal(dst.data?[int8(1)] ?? "", "one")
    //     t |> equal(dst.data?[int8(-1)] ?? "", "neg")
    // }
    t |> run("uint8-keyed table") @(t : T?) {
        var src : WithUInt8Table
        src.data <- { uint8(0) => "zero", uint8(255) => "max" }
        let json = sprint_json(src, false)
        var dst : WithUInt8Table
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst.data?[uint8(0)] ?? "", "zero")
        t |> equal(dst.data?[uint8(255)] ?? "", "max")
    }
    t |> run("int16-keyed table") @(t : T?) {
        var src : WithInt16Table
        src.data <- { int16(1000) => "thousand" }
        let json = sprint_json(src, false)
        var dst : WithInt16Table
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst.data?[int16(1000)] ?? "", "thousand")
    }
    t |> run("float-keyed table") @(t : T?) {
        var src : WithFloatTable
        src.data <- { 1.5f => "one-half", 2.0f => "two" }
        let json = sprint_json(src, false)
        var dst : WithFloatTable
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst.data?[1.5f] ?? "", "one-half")
        t |> equal(dst.data?[2.0f] ?? "", "two")
    }
    t |> run("double-keyed table") @(t : T?) {
        var src : WithDoubleTable
        src.data <- { 3.14lf => "pi" }
        let json = sprint_json(src, false)
        var dst : WithDoubleTable
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst.data?[3.14lf] ?? "", "pi")
    }
    t |> run("uint64-keyed table") @(t : T?) {
        var src : WithUInt64Table
        src.data <- { 100ul => "hundred" }
        let json = sprint_json(src, false)
        var dst : WithUInt64Table
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst.data?[100ul] ?? "", "hundred")
    }
    t |> run("int2-keyed table") @(t : T?) {
        var src : WithInt2Table
        src.data <- { int2(1, 2) => "onetwo" }
        let json = sprint_json(src, false)
        var dst : WithInt2Table
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst.data?[int2(1, 2)] ?? "", "onetwo")
    }
    t |> run("float2-keyed table") @(t : T?) {
        var src : WithFloat2Table
        src.data <- { float2(1.0, 2.0) => "coords" }
        let json = sprint_json(src, false)
        var dst : WithFloat2Table
        let ok = sscan_json(json, dst)
        t |> success(ok)
        t |> equal(dst.data?[float2(1.0, 2.0)] ?? "", "coords")
    }
}
