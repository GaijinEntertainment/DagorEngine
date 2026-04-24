options gen2
require dastest/testing_boost

require daslib/rtti

require daslib/fuzzer
require daslib/faker
require daslib/math
require daslib/constant_expression

// NOTE: I commented out tests in this file until we'll
// introduce new methods to fuzzer to generate bounded
// test cases. Otherwise UBSAN complains about
// value being outside of the type range.

////////////////////////////
// floating point type ctors

// float

def jit_float(x) {
    return float(x)
}

def test_float(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_float))
    t |> strictEqual(float(a), jit_float(a), "float(a)")
}

// [test]
def test_intrin_float(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_and_storage_op1(t, fake, "test_float")
    }
}

// double

def jit_double(x) {
    return double(x)
}

def test_double(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_double))
    t |> strictEqual(double(a), jit_double(a), "double(a)")
}

// [test]
def test_intrin_double(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_and_storage_op1(t, fake, "test_double")
    }
}

/////////////////////
// integer type ctors

// int

def jit_int(x) {
    return int(x)
}

def test_int(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_int))
    t |> strictEqual(int(a), jit_int(a), "int(a)")
}

// [test]
def test_intrin_int(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_and_storage_op1(t, fake, "test_int")
    }
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : CompilationError) : int > jit_int))
    t |> equal(int(CompilationError.function_not_found), jit_int(CompilationError.function_not_found))
}

// uint

def jit_uint(x) {
    return uint(x)
}

def test_uint(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_uint))
    t |> strictEqual(uint(a), jit_uint(a), "uint(a)")
}

// [test]
def test_intrin_uint(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_and_storage_op1(t, fake, "test_uint")
    }
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : CompilationError) : int > jit_uint))
    t |> equal(uint(CompilationError.function_not_found), jit_uint(CompilationError.function_not_found))
}


// int8

def jit_int8(x) {
    return int8(x)
}

def test_int8(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_int8))
    t |> strictEqual(int8(a), jit_int8(a), "int8(a)")
}

// [test]
def test_intrin_int8(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_and_storage_op1(t, fake, "test_int8")
    }
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : CompilationError) : int > jit_int8))
    t |> equal(int8(CompilationError.function_not_found), jit_int8(CompilationError.function_not_found))
}


// uint8

def jit_uint8(x) {
    return uint8(x)
}

def test_uint8(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_uint8))
    t |> strictEqual(uint8(a), jit_uint8(a), "uint8(a)")
}

// [test]
def test_intrin_uint8(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_and_storage_op1(t, fake, "test_uint8")
    }
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : CompilationError) : int > jit_uint8))
    t |> equal(uint8(CompilationError.function_not_found), jit_uint8(CompilationError.function_not_found))
}

// int16

def jit_int16(x) {
    return int16(x)
}

def test_int16(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_int16))
    t |> strictEqual(int16(a), jit_int16(a), "int16(a)")
}

// [test]
def test_intrin_int16(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_and_storage_op1(t, fake, "test_int16")
    }
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : CompilationError) : int > jit_int16))
    t |> equal(int16(CompilationError.function_not_found), jit_int16(CompilationError.function_not_found))
}


// uint16

def jit_uint16(x) {
    return uint16(x)
}

def test_uint16(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_uint16))
    t |> strictEqual(uint16(a), jit_uint16(a), "uint16(a)")
}

// [test]
def test_intrin_uint16(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_and_storage_op1(t, fake, "test_uint16")
    }
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : CompilationError) : int > jit_uint16))
    t |> equal(uint16(CompilationError.function_not_found), jit_uint16(CompilationError.function_not_found))
}


// int64

def jit_int64(x) {
    return int64(x)
}

def test_int64(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_int64))
    t |> strictEqual(int64(a), jit_int64(a), "int64(a)")
}

// [test]
def test_intrin_int64(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_and_storage_op1(t, fake, "test_int64")
    }
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : CompilationError) : int > jit_int64))
    t |> equal(int64(CompilationError.function_not_found), jit_int64(CompilationError.function_not_found))
}


// uint64

def jit_uint64(x) {
    return uint64(x)
}

def test_uint64(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_uint64))
    t |> strictEqual(uint64(a), jit_uint64(a), "uint64(a)")
}

// [test]
def test_intrin_uint64(t : T?) {
    var fake <- Faker()
    fuzz() {
        fuzz_numeric_and_storage_op1(t, fake, "test_uint64")
    }
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : CompilationError) : int > jit_uint64))
    t |> equal(uint64(CompilationError.function_not_found), jit_uint64(CompilationError.function_not_found))
}


///////////////
// vector ctors

// float4

def jit_float4(x) {
    return float4(x)
}

def test_float4(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_float4))
    t |> strictEqual(float4(a), jit_float4(a), "float4(a)")
}

[sideeffects]
def jit_float4_0 {
    return float4()
}

def jit_float4_2(a, b) {
    return float4(a, b)
}

def jit_float4_3(a, b, c) {
    return float4(a, b, c)
}

def jit_float4_4(x, y, z, w) {
    return float4(x, y, z, w)
}

def test_float4_4(t : T?; x, y, z, w : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (x, y, z, w : TT) : void > jit_float4_4))
    t |> strictEqual(float4(x, y, z, w), jit_float4_4(x, y, z, w), "float4(x,y,z,w)")
}

// [test]
def test_intrin_float4(t : T?) {
    t |> run("float4()") @(t : T?) {
        t |> strictEqual(float4(), jit_float4_0(), "float4()")
    }
    t |> run("float4(x)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_float4(t, fake |> random_int)
            test_float4(t, fake |> random_uint)
            test_float4(t, fake |> random_float)
            test_float4(t, fake |> random_double)
            test_float4(t, fake |> random_float4)
            test_float4(t, fake |> random_int4)
            test_float4(t, fake |> random_uint4)
        }
    }
    t |> run("float4(x,y)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            let f1 = fake |> random_float
            let f2 = fake |> random_float3
            t |> strictEqual(float4(f1, f2), jit_float4_2(f1, f2), "float4(x,y) // 1,3")
        }
        fuzz() {
            let f1 = fake |> random_float2
            let f2 = fake |> random_float2
            t |> strictEqual(float4(f1, f2), jit_float4_2(f1, f2), "float4(x,y) // 2,2")
        }
        fuzz() {
            let f1 = fake |> random_float3
            let f2 = fake |> random_float
            t |> strictEqual(float4(f1, f2), jit_float4_2(f1, f2), "float4(x,y) // 3,1")
        }
    }
    t |> run("float4(x,y,z)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            let f1 = fake |> random_float
            let f2 = fake |> random_float
            let f3 = fake |> random_float2
            t |> strictEqual(float4(f1, f2, f3), jit_float4_3(f1, f2, f3), "float4(x,y) // 1,1,2")
        }
        fuzz() {
            let f1 = fake |> random_float
            let f2 = fake |> random_float2
            let f3 = fake |> random_float
            t |> strictEqual(float4(f1, f2, f3), jit_float4_3(f1, f2, f3), "float4(x,y,z) // 1,2,1")
        }
    }
    t |> run("float4(x,y,z,w)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_op4(t, fake, "test_float4_4")
        }
    }
}

// float3

def jit_float3(x) {
    return float3(x)
}

def test_float3(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_float3))
    t |> strictEqual(float3(a), jit_float3(a), "float3(a)")
}

[sideeffects]
def jit_float3_0 {
    return float3()
}

def jit_float3_2(a, b) {
    return float3(a, b)
}

def jit_float3_3(a, b, c) {
    return float3(a, b, c)
}

def test_float3_3(t : T?; x, y, z : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (x, y, z : TT) : void > jit_float3_3))
    t |> strictEqual(float3(x, y, z), jit_float3_3(x, y, z), "float3(x,y,z)")
}

// [test]
def test_intrin_float3(t : T?) {
    t |> run("float3()") @(t : T?) {
        t |> strictEqual(float3(), jit_float3_0(), "float3()")
    }
    t |> run("float3(x)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_float3(t, fake |> random_int)
            test_float3(t, fake |> random_uint)
            test_float3(t, fake |> random_float)
            test_float3(t, fake |> random_double)
            test_float3(t, fake |> random_float3)
            test_float3(t, fake |> random_int3)
            test_float3(t, fake |> random_uint3)
        }
    }
    t |> run("float3(x,y)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            let f1 = fake |> random_float
            let f2 = fake |> random_float2
            t |> strictEqual(float3(f1, f2), jit_float3_2(f1, f2), "float3(x,y) // 1,2")
        }
        fuzz() {
            let f1 = fake |> random_float2
            let f2 = fake |> random_float
            t |> strictEqual(float3(f1, f2), jit_float3_2(f1, f2), "float3(x,y) // 2,1")
        }
    }
    t |> run("float3(x,y,z)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_op3(t, fake, "test_float3_3")
        }
    }
}

// float2

def jit_float2(x) {
    return float2(x)
}

def test_float2(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_float2))
    t |> strictEqual(float2(a), jit_float2(a), "float2(a)")
}

[sideeffects]
def jit_float2_0 {
    return float2()
}

def jit_float2_2(a, b) {
    return float2(a, b)
}

def test_float2_2(t : T?; x, y : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (x, y : TT) : void > jit_float2_2))
    t |> strictEqual(float2(x, y), jit_float2_2(x, y), "float2(x,y)")
}

// [test]
def test_intrin_float2(t : T?) {
    t |> run("float2()") @(t : T?) {
        t |> strictEqual(float2(), jit_float2_0(), "float2()")
    }
    t |> run("float2(x)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_float2(t, fake |> random_int)
            test_float2(t, fake |> random_uint)
            test_float2(t, fake |> random_float)
            test_float2(t, fake |> random_double)
            test_float2(t, fake |> random_float2)
            test_float2(t, fake |> random_int2)
            test_float2(t, fake |> random_uint2)
        }
    }
    t |> run("float2(x,y,z)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_op2(t, fake, "test_float2_2")
        }
    }
}

// int4

def jit_int4(x) {
    return int4(x)
}

def test_int4(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_int4))
    t |> strictEqual(int4(a), jit_int4(a), "int4(a)")
}

[sideeffects]
def jit_int4_0 {
    return int4()
}

def jit_int4_2(a, b) {
    return int4(a, b)
}

def jit_int4_3(a, b, c) {
    return int4(a, b, c)
}

def jit_int4_4(x, y, z, w) {
    return int4(x, y, z, w)
}

def test_int4_4(t : T?; x, y, z, w : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (x, y, z, w : TT) : void > jit_int4_4))
    t |> strictEqual(int4(x, y, z, w), jit_int4_4(x, y, z, w), "int4(x,y,z,w)")
}

// [test]
def test_intrin_int4(t : T?) {
    t |> run("int4()") @(t : T?) {
        t |> strictEqual(int4(), jit_int4_0(), "int4()")
    }
    t |> run("int4(x)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_int4(t, fake |> random_int)
            test_int4(t, fake |> random_uint)
            test_int4(t, fake |> random_float)
            test_int4(t, fake |> random_double)
            test_int4(t, fake |> random_float4)
            test_int4(t, fake |> random_int4)
            test_int4(t, fake |> random_uint4)
        }
    }
    t |> run("int4(x,y)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            let f1 = fake |> random_int
            let f2 = fake |> random_int3
            t |> strictEqual(int4(f1, f2), jit_int4_2(f1, f2), "int4(x,y) // 1,3")
        }
        fuzz() {
            let f1 = fake |> random_int2
            let f2 = fake |> random_int2
            t |> strictEqual(int4(f1, f2), jit_int4_2(f1, f2), "int4(x,y) // 2,2")
        }
        fuzz() {
            let f1 = fake |> random_int3
            let f2 = fake |> random_int
            t |> strictEqual(int4(f1, f2), jit_int4_2(f1, f2), "int4(x,y) // 3,1")
        }
    }
    t |> run("int4(x,y,z)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            let f1 = fake |> random_int
            let f2 = fake |> random_int
            let f3 = fake |> random_int2
            t |> strictEqual(int4(f1, f2, f3), jit_int4_3(f1, f2, f3), "int4(x,y) // 1,1,2")
        }
        fuzz() {
            let f1 = fake |> random_int
            let f2 = fake |> random_int2
            let f3 = fake |> random_int
            t |> strictEqual(int4(f1, f2, f3), jit_int4_3(f1, f2, f3), "int4(x,y,z) // 1,2,1")
        }
    }
    t |> run("int4(x,y,z,w)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_op4(t, fake, "test_int4_4")
        }
    }
}

// int3

def jit_int3(x) {
    return int3(x)
}

def test_int3(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_int3))
    t |> strictEqual(int3(a), jit_int3(a), "int3(a)")
}

[sideeffects]
def jit_int3_0 {
    return int3()
}

def jit_int3_2(a, b) {
    return int3(a, b)
}

def jit_int3_3(a, b, c) {
    return int3(a, b, c)
}

def test_int3_3(t : T?; x, y, z : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (x, y, z : TT) : void > jit_int3_3))
    t |> strictEqual(int3(x, y, z), jit_int3_3(x, y, z), "int3(x,y,z)")
}

// [test]
def test_intrin_int3(t : T?) {
    t |> run("int3()") @(t : T?) {
        t |> strictEqual(int3(), jit_int3_0(), "int3()")
    }
    t |> run("int3(x)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_int3(t, fake |> random_int)
            test_int3(t, fake |> random_uint)
            test_int3(t, fake |> random_float)
            test_int3(t, fake |> random_double)
            test_int3(t, fake |> random_float3)
            test_int3(t, fake |> random_int3)
            test_int3(t, fake |> random_uint3)
        }
    }
    t |> run("int3(x,y)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            let f1 = fake |> random_int
            let f2 = fake |> random_int2
            t |> strictEqual(int3(f1, f2), jit_int3_2(f1, f2), "int3(x,y) // 1,2")
        }
        fuzz() {
            let f1 = fake |> random_int2
            let f2 = fake |> random_int
            t |> strictEqual(int3(f1, f2), jit_int3_2(f1, f2), "int3(x,y) // 2,1")
        }
    }
    t |> run("int3(x,y,z)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_op3(t, fake, "test_int3_3")
        }
    }
}

// int2

def jit_int2(x) {
    return int2(x)
}

def test_int2(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_int2))
    t |> strictEqual(int2(a), jit_int2(a), "int2(a)")
}

[sideeffects]
def jit_int2_0 {
    return int2()
}

def jit_int2_2(a, b) {
    return int2(a, b)
}

def test_int2_2(t : T?; x, y : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (x, y : TT) : void > jit_int2_2))
    t |> strictEqual(int2(x, y), jit_int2_2(x, y), "int2(x,y)")
}

// [test]
def test_intrin_int2(t : T?) {
    t |> run("int2()") @(t : T?) {
        t |> strictEqual(int2(), jit_int2_0(), "int2()")
    }
    t |> run("int2(x)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_int2(t, fake |> random_int)
            test_int2(t, fake |> random_uint)
            test_int2(t, fake |> random_float)
            test_int2(t, fake |> random_double)
            test_int2(t, fake |> random_float2)
            test_int2(t, fake |> random_int2)
            test_int2(t, fake |> random_uint2)
        }
    }
    t |> run("int2(x,y)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_op2(t, fake, "test_int2_2")
        }
    }
}

// uint4

def jit_uint4(x) {
    return uint4(x)
}

def test_uint4(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_uint4))
    t |> strictEqual(uint4(a), jit_uint4(a), "uint4(a)")
}

[sideeffects]
def jit_uint4_0 {
    return uint4()
}

def jit_uint4_2(a, b) {
    return uint4(a, b)
}

def jit_uint4_3(a, b, c) {
    return uint4(a, b, c)
}

def jit_uint4_4(x, y, z, w) {
    return uint4(x, y, z, w)
}

def test_uint4_4(t : T?; x, y, z, w : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (x, y, z, w : TT) : void > jit_uint4_4))
    t |> strictEqual(uint4(x, y, z, w), jit_uint4_4(x, y, z, w), "uint4(x,y,z,w)")
}

// [test]
def test_intrin_uint4(t : T?) {
    t |> run("uint4()") @(t : T?) {
        t |> strictEqual(uint4(), jit_uint4_0(), "uint4()")
    }
    t |> run("uint4(x)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_uint4(t, fake |> random_int)
            test_uint4(t, fake |> random_uint)
            test_uint4(t, fake |> random_float)
            test_uint4(t, fake |> random_double)
            test_uint4(t, fake |> random_float4)
            test_uint4(t, fake |> random_int4)
            test_uint4(t, fake |> random_uint4)
        }
    }
    t |> run("uint4(x,y)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            let f1 = fake |> random_uint
            let f2 = fake |> random_uint3
            t |> strictEqual(uint4(f1, f2), jit_uint4_2(f1, f2), "uint4(x,y) // 1,3")
        }
        fuzz() {
            let f1 = fake |> random_uint2
            let f2 = fake |> random_uint2
            t |> strictEqual(uint4(f1, f2), jit_uint4_2(f1, f2), "uint4(x,y) // 2,2")
        }
        fuzz() {
            let f1 = fake |> random_uint3
            let f2 = fake |> random_uint
            t |> strictEqual(uint4(f1, f2), jit_uint4_2(f1, f2), "uint4(x,y) // 3,1")
        }
    }
    t |> run("uint4(x,y,z)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            let f1 = fake |> random_uint
            let f2 = fake |> random_uint
            let f3 = fake |> random_uint2
            t |> strictEqual(uint4(f1, f2, f3), jit_uint4_3(f1, f2, f3), "uint4(x,y) // 1,1,2")
        }
        fuzz() {
            let f1 = fake |> random_uint
            let f2 = fake |> random_uint2
            let f3 = fake |> random_uint
            t |> strictEqual(uint4(f1, f2, f3), jit_uint4_3(f1, f2, f3), "uint4(x,y,z) // 1,2,1")
        }
    }
    t |> run("uint4(x,y,z,w)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_op4(t, fake, "test_uint4_4")
        }
    }
}

// uint3

def jit_uint3(x) {
    return uint3(x)
}

def test_uint3(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_uint3))
    t |> strictEqual(uint3(a), jit_uint3(a), "uint3(a)")
}

[sideeffects]
def jit_uint3_0 {
    return uint3()
}

def jit_uint3_2(a, b) {
    return uint3(a, b)
}

def jit_uint3_3(a, b, c) {
    return uint3(a, b, c)
}

def test_uint3_3(t : T?; x, y, z : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (x, y, z : TT) : void > jit_uint3_3))
    t |> strictEqual(uint3(x, y, z), jit_uint3_3(x, y, z), "uint3(x,y,z)")
}

// [test]
def test_intrin_uint3(t : T?) {
    t |> run("uint3()") @(t : T?) {
        t |> strictEqual(uint3(), jit_uint3_0(), "uint3()")
    }
    t |> run("uint3(x)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_uint3(t, fake |> random_int)
            test_uint3(t, fake |> random_uint)
            test_uint3(t, fake |> random_float)
            test_uint3(t, fake |> random_double)
            test_uint3(t, fake |> random_float3)
            test_uint3(t, fake |> random_int3)
            test_uint3(t, fake |> random_uint3)
        }
    }
    t |> run("uint3(x,y)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            let f1 = fake |> random_uint
            let f2 = fake |> random_uint2
            t |> strictEqual(uint3(f1, f2), jit_uint3_2(f1, f2), "uint3(x,y) // 1,2")
        }
        fuzz() {
            let f1 = fake |> random_uint2
            let f2 = fake |> random_uint
            t |> strictEqual(uint3(f1, f2), jit_uint3_2(f1, f2), "uint3(x,y) // 2,1")
        }
    }
    t |> run("uint3(x,y,z)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_op3(t, fake, "test_uint3_3")
        }
    }
}

// uint2

def jit_uint2(x) {
    return uint2(x)
}

def test_uint2(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_uint2))
    t |> strictEqual(uint2(a), jit_uint2(a), "uint2({a}) // {typeinfo typename(type<TT>)}")
}

[sideeffects]
def jit_uint2_0 {
    return uint2()
}

def jit_uint2_2(a, b) {
    return uint2(a, b)
}

def test_uint2_2(t : T?; x, y : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (x, y : TT) : void > jit_uint2_2))
    t |> strictEqual(uint2(x, y), jit_uint2_2(x, y), "uint2(x,y)")
}

// [test]
def test_intrin_uint2(t : T?) {
    t |> run("uint2()") @(t : T?) {
        t |> strictEqual(uint2(), jit_uint2_0(), "uint2()")
    }
    t |> run("uint2(x)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_uint2(t, fake |> random_int)
            test_uint2(t, fake |> random_uint)
            test_uint2(t, fake |> random_float)
            test_uint2(t, fake |> random_double)
            test_uint2(t, fake |> random_float2)
            test_uint2(t, fake |> random_int2)
            test_uint2(t, fake |> random_uint2)
        }
    }
    t |> run("uint2(x,y)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_op2(t, fake, "test_uint2_2")
        }
    }
}


// range

def jit_range(x) {
    return range(x)
}

def test_range(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_range))
    t |> strictEqual(range(a), jit_range(a), "range(a)")
}

[sideeffects]
def jit_range_0 {
    return range()
}

def jit_range_2(a, b) {
    return range(a, b)
}

def test_range_2(t : T?; x, y : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (x, y : TT) : void > jit_range_2))
    t |> strictEqual(range(x, y), jit_range_2(x, y), "range(x,y)")
}

def jit_interval_2(a, b) {
    return a..b
}

def test_interval_2(t : T?; x, y : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (x, y : TT) : void > jit_interval_2))
    t |> strictEqual(x..y, jit_interval_2(x, y), "x..y")
}

// [test]
def test_intrin_range(t : T?) {
    t |> run("range()") @(t : T?) {
        t |> strictEqual(range(), jit_range_0(), "range()")
    }
    t |> run("range(x)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_range(t, fake |> random_int)
            test_range(t, fake |> random_uint)
            test_range(t, fake |> random_float)
            test_range(t, fake |> random_double)
            test_range(t, fake |> random_range)
        }
    }
    t |> run("range(x,y)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_op2(t, fake, "test_range_2")
        }
    }
    t |> run("x..y") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_interval_2(t, fake |> random_int, fake |> random_int)
        }
    }
}

// urange

def jit_urange(x) {
    return urange(x)
}

def test_urange(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_urange))
    t |> strictEqual(urange(a), jit_urange(a), "urange(a)")
}

[sideeffects]
def jit_urange_0 {
    return urange()
}

def jit_urange_2(a, b) {
    return urange(a, b)
}

def test_urange_2(t : T?; x, y : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (x, y : TT) : void > jit_urange_2))
    t |> strictEqual(urange(x, y), jit_urange_2(x, y), "urange(x,y)")
}

// [test]
def test_intrin_urange(t : T?) {
    t |> run("urange()") @(t : T?) {
        t |> strictEqual(urange(), jit_urange_0(), "urange()")
    }
    t |> run("urange(x)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_urange(t, fake |> random_int)
            test_urange(t, fake |> random_uint)
            test_urange(t, fake |> random_float)
            test_urange(t, fake |> random_double)
            test_urange(t, fake |> random_urange)
        }
    }
    t |> run("urange(x,y)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_op2(t, fake, "test_urange_2")
        }
    }
    t |> run("x..y") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_interval_2(t, fake |> random_uint, fake |> random_uint)
        }
    }
}

// range64

def jit_range64(x) {
    return range64(x)
}

def test_range64(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_range64))
    t |> strictEqual(range64(a), jit_range64(a), "range64(a)")
}

[sideeffects]
def jit_range64_0 {
    return range64()
}

def jit_range64_2(a, b) {
    return range64(a, b)
}

def test_range64_2(t : T?; x, y : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (x, y : TT) : void > jit_range64_2))
    t |> strictEqual(range64(x, y), jit_range64_2(x, y), "range64(x,y)")
}

// [test]
def test_intrin_range64(t : T?) {
    t |> run("range64()") @(t : T?) {
        t |> strictEqual(range64(), jit_range64_0(), "range64()")
    }
    t |> run("range64(x)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_range64(t, fake |> random_int)
            test_range64(t, fake |> random_uint)
            test_range64(t, fake |> random_float)
            test_range64(t, fake |> random_double)
            test_range64(t, fake |> random_range64)
        }
    }
    t |> run("range64(x,y)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_op2(t, fake, "test_range64_2")
        }
    }
    t |> run("x..y") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_interval_2(t, fake |> random_int64, fake |> random_int64)
        }
    }
}

// urange64

def jit_urange64(x) {
    return urange64(x)
}

def test_urange64(t : T?; a : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (a : TT) : void > jit_urange64))
    t |> strictEqual(urange64(a), jit_urange64(a), "urange64(a)")
}

[sideeffects]
def jit_urange64_0 {
    return urange64()
}

def jit_urange64_2(a, b) {
    return urange64(a, b)
}

def test_urange64_2(t : T?; x, y : auto(TT)) {
    t |> success(!jit_enabled() || is_jit_function(@@ < (x, y : TT) : void > jit_urange64_2))
    t |> strictEqual(urange64(x, y), jit_urange64_2(x, y), "urange64(x,y)")
}

// [test]
def test_intrin_urange64(t : T?) {
    t |> run("urange64()") @(t : T?) {
        t |> strictEqual(urange64(), jit_urange64_0(), "urange64()")
    }
    t |> run("urange64(x)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_urange64(t, fake |> random_int)
            test_urange64(t, fake |> random_uint)
            test_urange64(t, fake |> random_float)
            test_urange64(t, fake |> random_double)
            test_urange64(t, fake |> random_urange64)
        }
    }
    t |> run("urange64(x,y)") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            fuzz_numeric_op2(t, fake, "test_urange64_2")
        }
    }
    t |> run("x..y") @(t : T?) {
        var fake <- Faker()
        fuzz() {
            test_interval_2(t, fake |> random_uint64, fake |> random_uint64)
        }
    }
}
