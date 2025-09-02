options gen2

// Avoid optimizations
var _0 = 0
var _1 = 1
var _2 = 2
var _4 = 4
var _8 = 8
var _30 = 30
var _31 = 31
var _32 = 32
var _33 = 33
var _36 = 36

def test_shl_scalar {
    // Logical left shift (<<)
    assert(1 << _1 == 2)
    assert(1 << _31 == INT_MIN)
    assert(0xFFFFFFFF << uint(_0) == 0xFFFFFFFF)
    assert(1 << _32 == 1)
    assert(1u << uint(-_1) == 0x80000000)

    // Arithmetic right shift (>> for signed)
    let x = -8
    assert(x >> _1 == -_4)
    assert(x >> _31 == -_1)
    assert(x >> _32 == -_8)
    assert(x >> -_1 == -_1)

    // Logical right shift (>> for unsigned)
    let y = 0x80000000
    assert(y >> uint(_1) == 0x40000000)
    assert(y >> uint(_31) == 0x1)
    assert(y >> uint(_32) == 0x80000000)
    return true
}

[export]
def test_rotational_shifts {
    // === Scalar 32-bit rotation ===
    assert(0x12345678 <<< uint(_4)  == 0x23456781)
    assert(0x12345678 <<< uint(_36) == 0x23456781)
    assert(0x12345678 <<< uint(_0) == 0x12345678)
    assert(0x12345678 >>> uint(_0) == 0x12345678)

    var x1 = 0x12345678
    x1 >>>= uint(_0)
    assert(x1 == 0x12345678)
    x1 <<<= uint(_0)
    assert(x1 == 0x12345678)

    x1 >>>= uint(_4)
    assert(x1 == 0x81234567)
    var x2 = 0x12345678
    x2 <<<= uint(_36)
    assert(x2 == 0x23456781)
    var x3 = 0x12345678
    x3 <<<= uint(-_4)
    assert(x3 == 0x81234567)
    return true
}

def test_vector_shifts {
    // === Signed integer vectors (tInt2, tInt3, tInt4) ===
    let vi2 : int2 = int2(-8, 16)
    assert(vi2 << _2 == int2(-32, 64))
    assert(vi2 >> -_30 == int2(-2, 4))

    let vi3 : int3 = int3(-1l, 0x7FFFFFFFl, -0x80000000l)
    assert(vi3 >> -_31 == int3(-1, 0x3FFFFFFF, -1073741824))

    // === Unsigned integer vectors (tUInt2, tUInt3, tUInt4) ===
    let vu4 : uint4 = uint4(0x80000000, 0xC0000000, 0xE0000000, 0xF0000000)
    assert(vu4 >> _2 == uint4(0x20000000, 0x30000000, 0x38000000, 0x3c000000))

    // === Edge cases (zero/wrapping shifts) ===
    let vu2 : uint2 = uint2(0xFFFFFFFF, 0x12345678)
    assert(vu2 << _0 == uint2(0xFFFFFFFF, 0x12345678))
    assert(vu2 >> _33 == uint2(0x7FFFFFFF, 0x91a2b3c))

    return true
}

class A { x : int; }

def operator <<(x : A, y : int) { return new A(x = x.x << y); }
def operator >>(x : A, y : int) { return new A(x = x.x >> y); }
def operator <<=(var x : A, y : int) { x.x <<= y; }
def operator >>=(var x : A, y : int) { x.x >>= y; }

def operator <<(x : A, y : A) { return new A(x = x.x << y.x); }
def operator >>(x : A, y : A) { return new A(x = x.x >> y.x); }
def operator <<=(var x : A, y : A) { x.x <<= y.x; }
def operator >>=(var x : A, y : A) { x.x >>= y.x; }

def test_all_A_shifts {
    let a1 = new A(x = 8)
    let neg_a = new A(x = -8)

    // << and >> with int
    assert((*a1 << _1).x == 16)
    assert((*a1 >> _1).x == 4)
    assert((*neg_a >> _1).x == -4)
    assert((*a1 << _32).x == 8)
    assert((*a1 >> _33).x == 4)

    // <<= and >>= with int
    var a2 = new A(x = 8)
    *a2 <<= _1
    assert(a2.x == 16)

    var a3 = new A(x = 16)
    *a3 >>= _2
    assert(a3.x == 4)

    var a4 = new A(x = 1)
    *a4 <<= _31
    assert(a4.x == INT_MIN)
    *a4 <<= _1
    assert(a4.x == 0)

    // Test shifts with A operands
    let a5 = new A(x = 8)
    let shift_a = new A(x = 2)
    let zero_a = new A(x = 0)

    // << and >> with A
    assert((*a5 << *shift_a).x == 32)
    assert((*a5 >> *shift_a).x == 2)
    assert((*a5 << *zero_a).x == 8)
    assert((*neg_a >> *shift_a).x == -2)

    // <<= and >>= with A
    var a6 = new A(x = 16)
    let shift_a2 = new A(x = 3)
    *a6 <<= *shift_a2
    assert(a6.x == 128)
    *a6 >>= *shift_a2
    assert(a6.x == 16)
    return true
}

[export]
def test {
    test_shl_scalar()
    test_rotational_shifts()
    test_vector_shifts()
    test_all_A_shifts()
    return true
}