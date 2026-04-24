// Tests bit shift operators: <<, >>, <<<, >>>, compound assignments, vectors, custom operators
options gen2
require dastest/testing_boost public

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

def test_shl_scalar(t : T?) {
    t |> equal(1 << _1, 2)
    t |> equal(1 << _31, INT_MIN)
    t |> equal(0xFFFFFFFF << uint(_0), 0xFFFFFFFF)
    t |> equal(1 << _32, 1)
    t |> equal(1u << uint(-_1), 0x80000000)
    let x = -8
    t |> equal(x >> _1, -_4)
    t |> equal(x >> _31, -_1)
    t |> equal(x >> _32, -_8)
    t |> equal(x >> -_1, -_1)
    let y = 0x80000000
    t |> equal(y >> uint(_1), 0x40000000)
    t |> equal(y >> uint(_31), 0x1)
    t |> equal(y >> uint(_32), 0x80000000)
}

def test_rotational_shifts(t : T?) {
    t |> equal(0x12345678 <<< uint(_4), 0x23456781)
    t |> equal(0x12345678 <<< uint(_36), 0x23456781)
    t |> equal(0x12345678 <<< uint(_0), 0x12345678)
    t |> equal(0x12345678 >>> uint(_0), 0x12345678)
    var x1 = 0x12345678
    x1 >>>= uint(_0)
    t |> equal(x1, 0x12345678)
    x1 <<<= uint(_0)
    t |> equal(x1, 0x12345678)
    x1 >>>= uint(_4)
    t |> equal(x1, 0x81234567)
    var x2 = 0x12345678
    x2 <<<= uint(_36)
    t |> equal(x2, 0x23456781)
    var x3 = 0x12345678
    x3 <<<= uint(-_4)
    t |> equal(x3, 0x81234567)
}

def test_vector_shifts(t : T?) {
    let vi2 : int2 = int2(-8, 16)
    t |> equal(vi2 << _2, int2(-32, 64))
    t |> equal(vi2 >> -_30, int2(-2, 4))
    let vi3 : int3 = int3(-1l, 0x7FFFFFFFl, -0x80000000l)
    t |> equal(vi3 >> -_31, int3(-1, 0x3FFFFFFF, -1073741824))
    let vu4 : uint4 = uint4(0x80000000, 0xC0000000, 0xE0000000, 0xF0000000)
    t |> equal(vu4 >> _2, uint4(0x20000000, 0x30000000, 0x38000000, 0x3c000000))
    let vu2 : uint2 = uint2(0xFFFFFFFF, 0x12345678)
    t |> equal(vu2 << _0, uint2(0xFFFFFFFF, 0x12345678))
    t |> equal(vu2 >> _33, uint2(0x7FFFFFFF, 0x91a2b3c))
}

class ShiftA {
    x : int
}

def operator <<(a : ShiftA; y : int) { return new ShiftA(x = a.x << y); }
def operator >>(a : ShiftA; y : int) { return new ShiftA(x = a.x >> y); }
def operator <<=(var a : ShiftA; y : int) { a.x <<= y; }
def operator >>=(var a : ShiftA; y : int) { a.x >>= y; }
def operator <<(a : ShiftA; b : ShiftA) { return new ShiftA(x = a.x << b.x); }
def operator >>(a : ShiftA; b : ShiftA) { return new ShiftA(x = a.x >> b.x); }
def operator <<=(var a : ShiftA; b : ShiftA) { a.x <<= b.x; }
def operator >>=(var a : ShiftA; b : ShiftA) { a.x >>= b.x; }

def test_all_A_shifts(t : T?) {
    let a1 = new ShiftA(x = 8)
    let neg_a = new ShiftA(x = -8)
    t |> equal((*a1 << _1).x, 16)
    t |> equal((*a1 >> _1).x, 4)
    t |> equal((*neg_a >> _1).x, -4)
    t |> equal((*a1 << _32).x, 8)
    t |> equal((*a1 >> _33).x, 4)
    var a2 = new ShiftA(x = 8)
    *a2 <<= _1
    t |> equal(a2.x, 16)
    var a3 = new ShiftA(x = 16)
    *a3 >>= _2
    t |> equal(a3.x, 4)
    var a4 = new ShiftA(x = 1)
    *a4 <<= _31
    t |> equal(a4.x, INT_MIN)
    *a4 <<= _1
    t |> equal(a4.x, 0)
    let a5 = new ShiftA(x = 8)
    let shift_a = new ShiftA(x = 2)
    let zero_a = new ShiftA(x = 0)
    t |> equal((*a5 << *shift_a).x, 32)
    t |> equal((*a5 >> *shift_a).x, 2)
    t |> equal((*a5 << *zero_a).x, 8)
    t |> equal((*neg_a >> *shift_a).x, -2)
    var a6 = new ShiftA(x = 16)
    let shift_a2 = new ShiftA(x = 3)
    *a6 <<= *shift_a2
    t |> equal(a6.x, 128)
    *a6 >>= *shift_a2
    t |> equal(a6.x, 16)
}

[test]
def test_shifts(t : T?) {
    t |> run("scalar shift left/right") @(t : T?) { test_shl_scalar(t); }
    t |> run("rotational shifts") @(t : T?) { test_rotational_shifts(t); }
    t |> run("vector shifts") @(t : T?) { test_vector_shifts(t); }
    t |> run("custom operator shifts") @(t : T?) { test_all_A_shifts(t); }
}
