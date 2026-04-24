// Squirrel module under test: dagor.math (registered by sqratDagorMath.cpp)

from "dagor.math" import Point2, Point3, Point4, IPoint2, IPoint3, IPoint4, TMatrix, Quat, Color3, Color4, E3DCOLOR


// ============================================================
// Section 1: Point2/3/4 ctors, fields, ops
// ============================================================
println("--- Section 1: Point2/3/4 ctors, fields, ops ---")

let p2 = Point2(1.0, 2.0)
assert(p2.x == 1.0 && p2.y == 2.0, "Point2 fields")
let p2sum = p2 + Point2(3.0, 4.0)
assert(p2sum.x == 4.0 && p2sum.y == 6.0, "Point2 add")
let p2sub = p2sum - p2
assert(p2sub.x == 3.0 && p2sub.y == 4.0, "Point2 sub")

let p3 = Point3(1.0, 2.0, 3.0)
assert(p3.x == 1.0 && p3.y == 2.0 && p3.z == 3.0, "Point3 fields")
let p3scaled = p3 * 2.0
assert(p3scaled.x == 2.0 && p3scaled.y == 4.0 && p3scaled.z == 6.0, "Point3 scale")
let p3neg = -p3
assert(p3neg.x == -1.0 && p3neg.y == -2.0 && p3neg.z == -3.0, "Point3 unary minus")

let p4 = Point4(1.0, 2.0, 3.0, 4.0)
assert(p4.x == 1.0 && p4.w == 4.0, "Point4 fields")

println("Section 1 PASSED")


// ============================================================
// Section 2: lengthSq / length
// ============================================================
println("--- Section 2: lengthSq / length ---")

let v = Point3(3.0, 4.0, 0.0)
assert(v.lengthSq() == 25.0, $"lengthSq 3,4,0: {v.lengthSq()}")
let L = v.length()
assert(L > 4.99 && L < 5.01, $"length 3,4,0: {L}")

let v2 = Point2(3.0, 4.0)
let L2 = v2.length()
assert(L2 > 4.99 && L2 < 5.01, $"Point2 length: {L2}")

println("Section 2 PASSED")


// ============================================================
// Section 3: IPoint variants
// ============================================================
println("--- Section 3: IPoint2/3/4 ---")

let i2 = IPoint2(10, 20)
assert(i2.x == 10 && i2.y == 20, "IPoint2 fields")

let i3 = IPoint3(10, 20, 30)
assert(i3.x == 10 && i3.y == 20 && i3.z == 30, "IPoint3 fields")

let i4 = IPoint4(10, 20, 30, 40)
assert(i4.x == 10 && i4.w == 40, "IPoint4 fields")

println("Section 3 PASSED")


// ============================================================
// Section 4: TMatrix + Quat
// ============================================================
println("--- Section 4: TMatrix + Quat ---")

let tm = TMatrix()
assert(typeof tm == "TMatrix", "TMatrix instance type")

let q = Quat()
assert(typeof q == "Quat", "Quat default ctor")

let q4 = Quat(0.0, 0.0, 0.0, 1.0)
assert(q4.w == 1.0, "Quat 4-arg ctor w")

println("Section 4 PASSED")


// ============================================================
// Section 5: E3DCOLOR + Color3 + Color4
// ============================================================
println("--- Section 5: E3DCOLOR + Color3 + Color4 ---")

let c = E3DCOLOR(255, 128, 64, 255)
assert(c.r == 255 && c.g == 128 && c.b == 64 && c.a == 255, "E3DCOLOR fields")

let c3 = Color3(1.0, 0.5, 0.25)
assert(c3.r == 1.0 && c3.g == 0.5 && c3.b == 0.25, "Color3 fields")

let c4 = Color4(1.0, 0.5, 0.25, 0.125)
assert(c4.r == 1.0 && c4.a == 0.125, "Color4 fields")

println("Section 5 PASSED")


// ============================================================
// Section 6: incorrect input
// ============================================================
println("--- Section 6: incorrect input ---")

local threw = false
try {
  Point3("not a number", 0.0, 0.0)
} catch (e) {
  threw = true
}
assert(threw, "Point3 with non-numeric argument must throw")

threw = false
try {
  Quat(1.0, 2.0)
} catch (e) {
  threw = true
}
assert(threw, "Quat with 2-arg numeric must throw (only 0/1/4 supported)")

threw = false
try {
  E3DCOLOR(1, 2)
} catch (e) {
  threw = true
}
assert(threw, "E3DCOLOR with 2 ints must throw (1/3/4 args supported)")

println("Section 6 PASSED")


println("ALL TESTS PASSED")
