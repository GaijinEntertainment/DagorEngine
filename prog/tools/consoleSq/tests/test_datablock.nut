// Comprehensive DataBlock binding test script
// Exercises every safe/unsafe set/get method, constructors, loadFromText, sub-blocks.

import "DataBlock" as DataBlock
from "dagor.math" import Point2, Point3, Point4, IPoint3, IPoint4, TMatrix, E3DCOLOR

// ============================================================
// Section 1: Constructor, copy constructor, loadFromText
// ============================================================
println("--- Section 1: Constructor, copy constructor, loadFromText ---")

let blk = DataBlock()
assert(blk.paramCount() == 0, "empty ctor paramCount")
assert(blk.blockCount() == 0, "empty ctor blockCount")

// Copy constructor
blk.addStr("name", "hello")
blk.addInt("val", 42)
let blk2 = DataBlock(blk)
assert(blk2.getStr("name", "") == "hello", "copy ctor str")
assert(blk2.getInt("val", 0) == 42, "copy ctor int")

// loadFromText
let blk3 = DataBlock()
let txt = "strParam:t=\"abc\"\nintParam:i=99\nrealParam:r=1.5\nboolParam:b=yes\n"
blk3.loadFromText(txt, txt.len())
assert(blk3.getStr("strParam", "") == "abc", "loadFromText str")
assert(blk3.getInt("intParam", 0) == 99, "loadFromText int")
assert(blk3.getReal("realParam", 0.0) == 1.5, "loadFromText real")
assert(blk3.getBool("boolParam", false) == true, "loadFromText bool")

println("Section 1 PASSED")


// ============================================================
// Section 2: Add + Get round-trip for all 12 types
// ============================================================
println("--- Section 2: Add + Get round-trip for all 12 types ---")

let rt = DataBlock()

// Str
rt.addStr("s", "hello world")
assert(rt.getStr("s", "") == "hello world", "round-trip Str")

// Int
rt.addInt("i", 42)
assert(rt.getInt("i", 0) == 42, "round-trip Int")

// Real
rt.addReal("r", 3.14)
// float precision: compare within tolerance
let rval = rt.getReal("r", 0.0)
assert(rval > 3.13 && rval < 3.15, "round-trip Real")

// Bool
rt.addBool("b", true)
assert(rt.getBool("b", false) == true, "round-trip Bool")

// Point2
rt.addPoint2("p2", Point2(1.0, 2.0))
let p2 = rt.getPoint2("p2", Point2(0, 0))
assert(p2.x == 1.0 && p2.y == 2.0, "round-trip Point2")

// Point3
rt.addPoint3("p3", Point3(1.0, 2.0, 3.0))
let p3 = rt.getPoint3("p3", Point3(0, 0, 0))
assert(p3.x == 1.0 && p3.y == 2.0 && p3.z == 3.0, "round-trip Point3")

// Point4
rt.addPoint4("p4", Point4(1.0, 2.0, 3.0, 4.0))
let p4 = rt.getPoint4("p4", Point4(0, 0, 0, 0))
assert(p4.x == 1.0 && p4.y == 2.0 && p4.z == 3.0 && p4.w == 4.0, "round-trip Point4")

// IPoint3
rt.addIPoint3("ip3", IPoint3(10, 20, 30))
let ip3 = rt.getIPoint3("ip3", IPoint3(0, 0, 0))
assert(ip3.x == 10 && ip3.y == 20 && ip3.z == 30, "round-trip IPoint3")

// IPoint4
rt.addIPoint4("ip4", IPoint4(10, 20, 30, 40))
let ip4 = rt.getIPoint4("ip4", IPoint4(0, 0, 0, 0))
assert(ip4.x == 10 && ip4.y == 20 && ip4.z == 30 && ip4.w == 40, "round-trip IPoint4")

// TMatrix
let tm_in = TMatrix()
rt.addTm("tm", tm_in)
let tm_out = rt.getTm("tm", TMatrix())
assert(type(tm_out) == "instance" && typeof tm_out=="TMatrix", $"round-trip Tm type '{typeof tm_out}'")

// E3DCOLOR
let ec_s = E3DCOLOR(255, 0, 0, 255)
rt.addE3dcolor("ec", ec_s)
let ec_d = rt.getE3dcolor("ec", E3DCOLOR(0, 0, 0, 0))
assert(ec_d.r == ec_s.r && ec_d.g == ec_s.g && ec_d.b == ec_s.b && ec_d.a == ec_s.a, $"round-trip E3dcolor")

// Int64 via addInt with large value, read via getInt (safe)
rt.addInt("i64", 0x1FFFFFFFF)
assert(rt.getInt("i64") == 0x1FFFFFFFF, "round-trip Int64 via getInt")

println("Section 2 PASSED")


// ============================================================
// Section 3: Safe set mismatch for all 12 types
// ============================================================
println("--- Section 3: Safe set mismatch (12 methods) ---")

// Strategy: store as wrong type first, then safe-set with correct type.
// After safe-set, param has new type and value.

let sm = DataBlock()

// setStr on int param
sm.addInt("p_str", 99)
sm.setStr("p_str", "replaced")
assert(sm.getStr("p_str", "") == "replaced", "safe setStr mismatch")

// setInt on str param
sm.addStr("p_int", "wrong")
sm.setInt("p_int", 77)
assert(sm.getInt("p_int", 0) == 77, "safe setInt mismatch")

// setInt64 on str param
sm.addStr("p_int64", "wrong")
sm.setInt64("p_int64", 0x1FFFFFFFF)
assert(sm.getInt("p_int64") == 0x1FFFFFFFF, "safe setInt64 mismatch")

// setReal on str param
sm.addStr("p_real", "wrong")
sm.setReal("p_real", 2.5)
assert(sm.getReal("p_real", 0.0) == 2.5, "safe setReal mismatch")

// setBool on str param
sm.addStr("p_bool", "wrong")
sm.setBool("p_bool", true)
assert(sm.getBool("p_bool", false) == true, "safe setBool mismatch")

// setPoint2 on int param
sm.addInt("p_p2", 1)
sm.setPoint2("p_p2", Point2(5.0, 6.0))
let sp2 = sm.getPoint2("p_p2", Point2(0, 0))
assert(sp2.x == 5.0 && sp2.y == 6.0, "safe setPoint2 mismatch")

// setPoint3 on int param
sm.addInt("p_p3", 1)
sm.setPoint3("p_p3", Point3(5.0, 6.0, 7.0))
let sp3 = sm.getPoint3("p_p3", Point3(0, 0, 0))
assert(sp3.x == 5.0 && sp3.y == 6.0 && sp3.z == 7.0, "safe setPoint3 mismatch")

// setPoint4 on int param
sm.addInt("p_p4", 1)
sm.setPoint4("p_p4", Point4(5.0, 6.0, 7.0, 8.0))
let sp4 = sm.getPoint4("p_p4", Point4(0, 0, 0, 0))
assert(sp4.x == 5.0 && sp4.y == 6.0 && sp4.z == 7.0 && sp4.w == 8.0, "safe setPoint4 mismatch")

// setIPoint3 on str param
sm.addStr("p_ip3", "wrong")
sm.setIPoint3("p_ip3", IPoint3(10, 20, 30))
let sip3 = sm.getIPoint3("p_ip3", IPoint3(0, 0, 0))
assert(sip3.x == 10 && sip3.y == 20 && sip3.z == 30, "safe setIPoint3 mismatch")

// setIPoint4 on str param
sm.addStr("p_ip4", "wrong")
sm.setIPoint4("p_ip4", IPoint4(10, 20, 30, 40))
let sip4 = sm.getIPoint4("p_ip4", IPoint4(0, 0, 0, 0))
assert(sip4.x == 10 && sip4.y == 20 && sip4.z == 30 && sip4.w == 40, "safe setIPoint4 mismatch")

// setTm on str param
sm.addStr("p_tm", "wrong")
sm.setTm("p_tm", TMatrix())
let stm = sm.getTm("p_tm", TMatrix())
assert(type(stm) == "instance" && typeof stm == "TMatrix", "safe setTm mismatch")

// setE3dcolor on str param
sm.addStr("p_ec", "wrong")
sm.setE3dcolor("p_ec", E3DCOLOR(128, 64, 32, 255))
let p_ec = sm.getE3dcolor("p_ec", E3DCOLOR(0,0,0,0))
assert(p_ec.r == 128 && p_ec.g==64 && p_ec.b==32, "safe setE3dcolor mismatch")

println("Section 3 PASSED")


// ============================================================
// Section 4: Safe get mismatch (11 methods — no getInt64)
// ============================================================
println("--- Section 4: Safe get mismatch (11 methods) ---")

// Strategy: store as one type, read via a different getX with a default.
// Should return the default, not the stored value.

let gm = DataBlock()

// getStr on int param → returns default
gm.addInt("gm_str", 42)
assert(gm.getStr("gm_str", "default") == "default", "safe getStr mismatch returns default")

// getInt on str param → returns default
gm.addStr("gm_int", "hello")
assert(gm.getInt("gm_int", -1) == -1, "safe getInt mismatch returns default")

// getReal on str param → returns default
gm.addStr("gm_real", "hello")
assert(gm.getReal("gm_real", 9.9) == 9.9, "safe getReal mismatch returns default")

// getBool on str param → returns default
gm.addStr("gm_bool", "hello")
assert(gm.getBool("gm_bool", true) == true, "safe getBool mismatch returns default")

// getPoint2 on int param → returns default
gm.addInt("gm_p2", 1)
let gp2 = gm.getPoint2("gm_p2", Point2(99.0, 88.0))
assert(gp2.x == 99.0 && gp2.y == 88.0, "safe getPoint2 mismatch returns default")

// getPoint3 on int param → returns default
gm.addInt("gm_p3", 1)
let gp3 = gm.getPoint3("gm_p3", Point3(99.0, 88.0, 77.0))
assert(gp3.x == 99.0 && gp3.y == 88.0 && gp3.z == 77.0, "safe getPoint3 mismatch returns default")

// getPoint4 on int param → returns default
gm.addInt("gm_p4", 1)
let gp4 = gm.getPoint4("gm_p4", Point4(99.0, 88.0, 77.0, 66.0))
assert(gp4.x == 99.0 && gp4.y == 88.0 && gp4.z == 77.0 && gp4.w == 66.0, "safe getPoint4 mismatch returns default")

// getIPoint3 on str param → returns default
gm.addStr("gm_ip3", "hello")
let gip3 = gm.getIPoint3("gm_ip3", IPoint3(91, 82, 73))
assert(gip3.x == 91 && gip3.y == 82 && gip3.z == 73, "safe getIPoint3 mismatch returns default")

// getIPoint4 on str param → returns default
gm.addStr("gm_ip4", "hello")
let gip4 = gm.getIPoint4("gm_ip4", IPoint4(91, 82, 73, 64))
assert(gip4.x == 91 && gip4.y == 82 && gip4.z == 73 && gip4.w == 64, "safe getIPoint4 mismatch returns default")

// getTm on int param → returns default
gm.addInt("gm_tm", 1)
let gtm = gm.getTm("gm_tm", TMatrix())
assert(typeof gtm == "TMatrix", "safe getTm mismatch returns default")

// getE3dcolor on str param → returns default
gm.addStr("gm_ec", "hello")
let gm_ec = gm.getE3dcolor("gm_ec", E3DCOLOR(11,22,33,44))
assert( gm_ec.r == 11 && gm_ec.g == 22 && gm_ec.b==33 && gm_ec.a==44, "safe getE3dcolor mismatch returns default")

println("Section 4 PASSED")




// ============================================================
// Section 5: Int/Int64 compatibility
// ============================================================
println("--- Section 5: Int/Int64 compatibility ---")

let ic = DataBlock()

// TYPE_INT (small value) read via getInt -- no mismatch
ic.addInt("small", 42)
assert(ic.getInt("small", 0) == 42, "int via getInt")

// TYPE_INT64 (large value) read via getInt -- cross-read allowed
ic.addInt("large", 0x1FFFFFFFF)
assert(ic.getInt("large") == 0x1FFFFFFFF, "int64 via getInt cross-read")

// getInt64 on TYPE_INT param -- cross-read allowed
assert(ic.getInt64("small", 0) == 42, "int via getInt64 cross-read")

// setInt (small value) on int64 param -- replaces with TYPE_INT (logwarn)
ic.setInt("large", 100)
assert(ic.getInt("large") == 100, "setInt on int64 param -- replaces with exact type")

// setInt64 on int param -- replaces with TYPE_INT64 (logwarn)
ic.setInt64("small", 0x2FFFFFFFF)
assert(ic.getInt("small") == 0x2FFFFFFFF, "setInt64 on int param -- replaces with exact type")

// setInt with large value dispatches to int64
ic.addInt("dispatch", 1)
ic.setInt("dispatch", 0x3FFFFFFFF)
assert(ic.getInt("dispatch") == 0x3FFFFFFFF, "setInt large value dispatches to int64")

println("Section 5 PASSED")


// ============================================================
// Section 6: addBlock/addNewBlock + removeParam/removeBlock
// ============================================================
println("--- Section 6: addBlock/addNewBlock, removeParam, removeBlock ---")

let bb = DataBlock()
assert(bb.blockCount() == 0, "initial blockCount 0")

// addBlock
let sub1 = bb.addBlock("child1")
assert(bb.blockCount() == 1, "blockCount after addBlock")
assert(typeof sub1 == "DataBlock", "addBlock returns instance")
sub1.addStr("key", "val1")
assert(sub1.getStr("key", "") == "val1", "sub-block addStr/getStr")

// addNewBlock with same name creates a second block
let sub2 = bb.addNewBlock("child1")
assert(bb.blockCount() == 2, "blockCount after addNewBlock same name")
sub2.addInt("num", 123)
assert(sub2.getInt("num", 0) == 123, "new sub-block addInt/getInt")

// addBlock with existing name returns existing block
let sub1_again = bb.addBlock("child1")
assert(sub1_again.getStr("key", "") == "val1", "addBlock existing returns same block")
assert(bb.blockCount() == 2, "blockCount unchanged after addBlock existing")

// removeBlock
assert(bb.removeBlock("child1") == true, "removeBlock returns true")
assert(bb.blockCount() == 0, $"blockCount after removeBlock: {bb.blockCount()}")

// removeParam
bb.addStr("tmp_param", "abc")
assert(bb.paramCount() >= 1, "paramCount after addStr")
assert(bb.removeParam("tmp_param") == true, "removeParam returns true")
assert(bb.getStr("tmp_param", "gone") == "gone", "param removed — returns default")

// removeParam non-existent
assert(bb.removeParam("no_such_param") == false, "removeParam non-existent returns false")

// removeBlock non-existent
assert(bb.removeBlock("no_such_block") == false, "removeBlock non-existent returns false")

println("Section 6 PASSED")


// ============================================================
// FINAL
// ============================================================
println("ALL TESTS PASSED")
