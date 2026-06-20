// Stage 0 characterization for the tFixedArray rework (FIXED_ARRAY_REWORK.md).
// Pins memory layout of fixed arrays: sizes, alignment, element stride, field offsets.
// C++ interop depends on this layout staying byte-identical — any diff after the
// representation flip is a stop-the-line bug.
options gen2
require dastest/testing_boost public

struct Packed {
    a : int8
    b : int
    c : int8
}

struct WithFA {
    tag : int8
    data : float[3]
    tail : int8
}

variant VarFA {
    ints : int[4]
    one : float
}

[test]
def test_sizes(t : T?) {
    t |> run("scalar element sizes") @(t : T?) {
        var _i4 : int[4]
        var _b3 : int8[3]
        var _d2 : double[2]
        t |> equal(typeinfo sizeof(_i4), 16)
        t |> equal(typeinfo alignof(_i4), 4)
        t |> equal(typeinfo sizeof(_b3), 3)     // no padding between int8 elements
        t |> equal(typeinfo alignof(_b3), 1)
        t |> equal(typeinfo sizeof(_d2), 16)
        t |> equal(typeinfo alignof(_d2), 8)
    }
    t |> run("multi-dim sizes multiply") @(t : T?) {
        var _m : float[2][3]
        var _c : int[2][3][4]
        t |> equal(typeinfo sizeof(_m), 24)
        t |> equal(typeinfo sizeof(_c), 96)
    }
    t |> run("vector elements use packed stride") @(t : T?) {
        // float3 inside a fixed array packs to 12 bytes, NOT the standalone 16-byte
        // vec4f — by design, memory matters; loads go through v_ldu when unaligned
        // vector loads are explicitly enabled (asan off)
        var _v3 : float3[4]
        var _v2 : float2[3]
        var _v4 : float4[2]
        t |> equal(typeinfo sizeof(_v3), 48)
        t |> equal(typeinfo alignof(_v3), 4)
        t |> equal(typeinfo sizeof(_v2), 24)
        t |> equal(typeinfo sizeof(_v4), 32)
    }
    t |> run("struct element stride = struct size") @(t : T?) {
        var _ps : Packed[2]
        t |> equal(typeinfo sizeof(type<Packed>), 12)
        t |> equal(typeinfo sizeof(_ps), 24)
    }
}

[test]
def test_addresses(t : T?) {
    t |> run("element stride via addresses") @(t : T?) {
        var s : Packed[2]
        unsafe {
            let p0 = intptr(addr(s[0]))
            let p1 = intptr(addr(s[1]))
            t |> equal(int(p1 - p0), 12)
        }
        var m : float[2][3]
        unsafe {
            let r0 = intptr(addr(m[0]))
            let r1 = intptr(addr(m[1]))
            t |> equal(int(r1 - r0), 12)        // row stride = 3 floats
            let e0 = intptr(addr(m[0][0]))
            let e1 = intptr(addr(m[0][1]))
            t |> equal(int(e1 - e0), 4)
        }
    }
    t |> run("2D rows are contiguous") @(t : T?) {
        var m : float[2][3]
        unsafe {
            let lastOfRow0 = intptr(addr(m[0][2]))
            let firstOfRow1 = intptr(addr(m[1][0]))
            t |> equal(int(firstOfRow1 - lastOfRow0), 4)
        }
    }
    t |> run("FA field offsets inside struct") @(t : T?) {
        var w : WithFA
        unsafe {
            let base = intptr(addr(w.tag))
            t |> equal(int(intptr(addr(w.data[0])) - base), 4)   // aligned to float
            t |> equal(int(intptr(addr(w.data[2])) - base), 12)
            t |> equal(int(intptr(addr(w.tail)) - base), 16)
        }
        t |> equal(typeinfo sizeof(type<WithFA>), 20)
    }
}

[test]
def test_composite_layout(t : T?) {
    t |> run("tuple with FA member") @(t : T?) {
        var tp : tuple<flag : int8; arr : int[3]>
        t |> equal(typeinfo sizeof(tp), 16)
        unsafe {
            let base = intptr(addr(tp.flag))
            t |> equal(int(intptr(addr(tp.arr[0])) - base), 4)
        }
    }
    t |> run("variant with FA arm") @(t : T?) {
        var vf : VarFA
        t |> equal(typeinfo sizeof(vf), 20)     // 16 payload + index
        unsafe {
            vf.ints[1] = 7
        }
        t |> success(vf is ints)
        t |> equal((vf as ints)[1], 7)
    }
    t |> run("FA of tuples") @(t : T?) {
        var ta : tuple<a : int; b : int8>[3]
        t |> equal(typeinfo sizeof(ta), 24)     // tuple size 8 (padded), x3
        unsafe {
            let p0 = intptr(addr(ta[0]))
            let p1 = intptr(addr(ta[1]))
            t |> equal(int(p1 - p0), 8)
        }
    }
}
