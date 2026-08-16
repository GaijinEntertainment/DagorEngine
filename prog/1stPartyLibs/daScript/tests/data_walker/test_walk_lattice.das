// Tests for DapiDataWalker — 16/8-bit type lattice leaves
//
// Covers: Float16, Half2/3/4/8, Short2/3/4/8, UShort2/3/4/8,
//         Byte2/3/4/8/16, UByte2/3/4/8/16 dispatch to das-side walkers,
//         lattice fields inside structs, fixed/dynamic arrays of lattice
//         elements, and mutation through the by-ref leaf callbacks

options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false

require dw_common


// ============================================================
// Local data types
// ============================================================

struct LatticeStruct {
    h : float16
    hv : half4
    sv : short2
    bv : byte4
    ubv : ubyte16
}


// ============================================================
// Local walker classes
// ============================================================

// Overwrites lattice leaves through the by-ref callback
class LatticeMutator : DapiDataWalker {
    def override Half2(var value : half2&) : void {
        value = half2(9., 9.)
    }
    def override Byte4(var value : byte4&) : void {
        value = byte4(1, 2, 3, 4)
    }
}


// ============================================================
// Standalone leaf dispatch
// ============================================================

[test]
def test_walk_lattice_leaves(t : T?) {
    t |> run("half family") @(t : T?) {
        var walker = new LogWalker()
        var f16 = float16(1.5)
        var h2 = half2(1., 2.)
        var h3 = half3(1., 2., 3.)
        var h4 = half4(1., 2., 3., 4.)
        var h8 = half8(1., 2., 3., 4., 5., 6., 7., 8.)
        var expected = ""
        make_data_walker(walker) $(adapter) {
            unsafe {
                adapter |> walk_data(addr(f16), typeinfo rtti_typeinfo(f16))
                adapter |> walk_data(addr(h2), typeinfo rtti_typeinfo(h2))
                adapter |> walk_data(addr(h3), typeinfo rtti_typeinfo(h3))
                adapter |> walk_data(addr(h4), typeinfo rtti_typeinfo(h4))
                adapter |> walk_data(addr(h8), typeinfo rtti_typeinfo(h8))
            }
        }
        expected += "Float16:{f16}\n"
        expected += "Half2:{h2}\n"
        expected += "Half3:{h3}\n"
        expected += "Half4:{h4}\n"
        expected += "Half8:{h8}\n"
        t |> equal(walker.log, expected)
        unsafe { delete walker; }
    }
    t |> run("short family") @(t : T?) {
        var walker = new LogWalker()
        var s2 = short2(-1, 2)
        var s3 = short3(-1, 2, -3)
        var s4 = short4(-1, 2, -3, 4)
        var s8 = short8(-1, 2, -3, 4, -5, 6, -7, 8)
        var u2 = ushort2(1, 2)
        var u3 = ushort3(1, 2, 3)
        var u4 = ushort4(1, 2, 3, 4)
        var u8 = ushort8(1, 2, 3, 4, 5, 6, 7, 8)
        var expected = ""
        make_data_walker(walker) $(adapter) {
            unsafe {
                adapter |> walk_data(addr(s2), typeinfo rtti_typeinfo(s2))
                adapter |> walk_data(addr(s3), typeinfo rtti_typeinfo(s3))
                adapter |> walk_data(addr(s4), typeinfo rtti_typeinfo(s4))
                adapter |> walk_data(addr(s8), typeinfo rtti_typeinfo(s8))
                adapter |> walk_data(addr(u2), typeinfo rtti_typeinfo(u2))
                adapter |> walk_data(addr(u3), typeinfo rtti_typeinfo(u3))
                adapter |> walk_data(addr(u4), typeinfo rtti_typeinfo(u4))
                adapter |> walk_data(addr(u8), typeinfo rtti_typeinfo(u8))
            }
        }
        expected += "Short2:{s2}\n"
        expected += "Short3:{s3}\n"
        expected += "Short4:{s4}\n"
        expected += "Short8:{s8}\n"
        expected += "UShort2:{u2}\n"
        expected += "UShort3:{u3}\n"
        expected += "UShort4:{u4}\n"
        expected += "UShort8:{u8}\n"
        t |> equal(walker.log, expected)
        unsafe { delete walker; }
    }
    t |> run("byte family") @(t : T?) {
        var walker = new LogWalker()
        var b2 = byte2(-1, 2)
        var b3 = byte3(-1, 2, -3)
        var b4 = byte4(-1, 2, -3, 4)
        var b8 = byte8(-1, 2, -3, 4, -5, 6, -7, 8)
        var b16 = byte16(-1, 2, -3, 4, -5, 6, -7, 8, -9, 10, -11, 12, -13, 14, -15, 16)
        var ub2 = ubyte2(1, 2)
        var ub3 = ubyte3(1, 2, 3)
        var ub4 = ubyte4(1, 2, 3, 4)
        var ub8 = ubyte8(1, 2, 3, 4, 5, 6, 7, 8)
        var ub16 = ubyte16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)
        var expected = ""
        make_data_walker(walker) $(adapter) {
            unsafe {
                adapter |> walk_data(addr(b2), typeinfo rtti_typeinfo(b2))
                adapter |> walk_data(addr(b3), typeinfo rtti_typeinfo(b3))
                adapter |> walk_data(addr(b4), typeinfo rtti_typeinfo(b4))
                adapter |> walk_data(addr(b8), typeinfo rtti_typeinfo(b8))
                adapter |> walk_data(addr(b16), typeinfo rtti_typeinfo(b16))
                adapter |> walk_data(addr(ub2), typeinfo rtti_typeinfo(ub2))
                adapter |> walk_data(addr(ub3), typeinfo rtti_typeinfo(ub3))
                adapter |> walk_data(addr(ub4), typeinfo rtti_typeinfo(ub4))
                adapter |> walk_data(addr(ub8), typeinfo rtti_typeinfo(ub8))
                adapter |> walk_data(addr(ub16), typeinfo rtti_typeinfo(ub16))
            }
        }
        expected += "Byte2:{b2}\n"
        expected += "Byte3:{b3}\n"
        expected += "Byte4:{b4}\n"
        expected += "Byte8:{b8}\n"
        expected += "Byte16:{b16}\n"
        expected += "UByte2:{ub2}\n"
        expected += "UByte3:{ub3}\n"
        expected += "UByte4:{ub4}\n"
        expected += "UByte8:{ub8}\n"
        expected += "UByte16:{ub16}\n"
        t |> equal(walker.log, expected)
        unsafe { delete walker; }
    }
}


// ============================================================
// Lattice fields inside a struct
// ============================================================

[test]
def test_walk_lattice_struct(t : T?) {
    t |> run("struct with lattice fields") @(t : T?) {
        var walker = new LogWalker()
        var s = LatticeStruct(
            h = float16(1.5),
            hv = half4(1., 2., 3., 4.),
            sv = short2(-1, 2),
            bv = byte4(-1, 2, -3, 4),
            ubv = ubyte16(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16))
        make_data_walker(walker) $(adapter) {
            unsafe {
                adapter |> walk_data(addr(s), typeinfo rtti_typeinfo(s))
            }
        }
        t |> equal(find(walker.log, "beforeStruct:LatticeStruct") != -1, true)
        t |> equal(find(walker.log, "Float16:{s.h}") != -1, true)
        t |> equal(find(walker.log, "Half4:{s.hv}") != -1, true)
        t |> equal(find(walker.log, "Short2:{s.sv}") != -1, true)
        t |> equal(find(walker.log, "Byte4:{s.bv}") != -1, true)
        t |> equal(find(walker.log, "UByte16:{s.ubv}") != -1, true)
        t |> equal(find(walker.log, "afterStruct:LatticeStruct") != -1, true)
        unsafe { delete walker; }
    }
}


// ============================================================
// Arrays of lattice elements
// ============================================================

[test]
def test_walk_lattice_arrays(t : T?) {
    t |> run("fixed array of half4") @(t : T?) {
        var walker = new LogWalker()
        var a : half4[2]
        a[0] = half4(1., 2., 3., 4.)
        a[1] = half4(5., 6., 7., 8.)
        make_data_walker(walker) $(adapter) {
            unsafe {
                adapter |> walk_data(addr(a), typeinfo rtti_typeinfo(a))
            }
        }
        t |> equal(find(walker.log, "beforeDim") != -1, true)
        t |> equal(find(walker.log, "Half4:{a[0]}") != -1, true)
        t |> equal(find(walker.log, "Half4:{a[1]}") != -1, true)
        t |> equal(find(walker.log, "afterDim") != -1, true)
        unsafe { delete walker; }
    }
    t |> run("dynamic array of byte4") @(t : T?) {
        var walker = new LogWalker()
        var arr = [byte4(1, 2, 3, 4), byte4(5, 6, 7, 8)]
        make_data_walker(walker) $(adapter) {
            unsafe {
                adapter |> walk_data(addr(arr), typeinfo rtti_typeinfo(arr))
            }
        }
        t |> equal(find(walker.log, "beforeArray") != -1, true)
        t |> equal(find(walker.log, "Byte4:{arr[0]}") != -1, true)
        t |> equal(find(walker.log, "Byte4:{arr[1]}") != -1, true)
        t |> equal(find(walker.log, "afterArray") != -1, true)
        delete arr
        unsafe { delete walker; }
    }
}


// ============================================================
// Mutation through by-ref leaves
// ============================================================

[test]
def test_walk_lattice_mutation(t : T?) {
    t |> run("mutate half2 and byte4") @(t : T?) {
        var walker = new LatticeMutator()
        var h = half2(1., 2.)
        var b = byte4(9, 9, 9, 9)
        make_data_walker(walker) $(adapter) {
            unsafe {
                adapter |> walk_data(addr(h), typeinfo rtti_typeinfo(h))
                adapter |> walk_data(addr(b), typeinfo rtti_typeinfo(b))
            }
        }
        t |> equal(h == half2(9., 9.), true)
        t |> equal(b == byte4(1, 2, 3, 4), true)
        unsafe { delete walker; }
    }
}
