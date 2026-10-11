options gen2
require dastest/testing_boost

// a make-array literal whose elements are all numeric or 2-/4-wide vector constants lowers to constant
// data and one memcpy in the JIT; every element width, the 3-wide vector the lowering excludes, a literal
// under the four-element floor and one with a runtime element read back element for element

def sum_i8(a : int8[5]) : int {
    var s = 0
    for (v in a) {
        s += int(v)
    }
    return s
}

def sum_i16(a : int16[5]) : int {
    var s = 0
    for (v in a) {
        s += int(v)
    }
    return s
}

def sum_f4(a : float4[4]) : float {
    var s = 0.0
    for (v in a) {
        s += v.x + v.y + v.z + v.w
    }
    return s
}

def sum_f3(a : float3[4]) : float {
    var s = 0.0
    for (v in a) {
        s += v.x + v.y + v.z
    }
    return s
}

def sum_d(a : double[4]) : double {
    var s = 0.0lf
    for (v in a) {
        s += v
    }
    return s
}

def sum_i(a : int[3]) : int {
    var s = 0
    for (v in a) {
        s += v
    }
    return s
}

def sum_i4(a : int[4]) : int {
    var s = 0
    for (v in a) {
        s += v
    }
    return s
}

[test]
def test_const_data_literal(t : T?) {
    t |> run("one-byte and two-byte elements") @(t : T?) {
        let bytes = fixed_array<int8>(int8(1), int8(-2), int8(3), int8(-4), int8(5))
        t |> equal(3, sum_i8(bytes))
        t |> equal(int8(-4), bytes[3])
        let halves = fixed_array<int16>(int16(1000), int16(-2000), int16(3000), int16(-4000), int16(5000))
        t |> equal(3000, sum_i16(halves))
        t |> equal(int16(5000), halves[4])
    }
    t |> run("16-byte vector elements, and the 3-wide vector the lowering excludes") @(t : T?) {
        let quads = fixed_array<float4>(float4(1, 2, 3, 4), float4(5, 6, 7, 8), float4(9, 10, 11, 12), float4(13, 14, 15, 16))
        t |> equal(136.0, sum_f4(quads))
        t |> equal(float4(13, 14, 15, 16), quads[3])
        let triples = fixed_array<float3>(float3(1, 2, 3), float3(4, 5, 6), float3(7, 8, 9), float3(10, 11, 12))
        t |> equal(78.0, sum_f3(triples))
        t |> equal(float3(10, 11, 12), triples[3])
    }
    t |> run("eight-byte elements") @(t : T?) {
        let wide = fixed_array<double>(1.5lf, -2.5lf, 3.5lf, -4.5lf)
        t |> equal(-2.0lf, sum_d(wide))
        t |> equal(-4.5lf, wide[3])
    }
    t |> run("under the floor, and a runtime element among constants") @(t : T?) {
        let three = fixed_array<int>(10, 20, 30)
        t |> equal(60, sum_i(three))
        let n = length(three)
        let mixed = fixed_array<int>(1, 2, n, 4)
        t |> equal(10, sum_i4(mixed))
        t |> equal(3, mixed[2])
    }
}
