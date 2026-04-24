// Test native field access (MEMBER_TYPE_NATIVE_FIELD)
// NativeVec has float x,y,z and int32 w as native fields

from "test.native" import NativeVec

// Basic read after construction
local v = NativeVec(1.0, 2.0, 3.0, 42)
println($"construct: {v.x} {v.y} {v.z} {v.w}")

// Write float fields
v.x = 10.0
v.y = 20.0
v.z = 30.0
println($"write float: {v.x} {v.y} {v.z}")

// Write int field
v.w = 100
println($"write int: {v.w}")

// Type coercion: assign int to float field
v.x = 7
println($"int to float: {v.x}")

// Type coercion: assign float to int field
v.w = 3.14
println($"float to int: {v.w}")

// Default construction (zeros)
local v2 = NativeVec()
println($"default: {v2.x} {v2.y} {v2.z} {v2.w}")

// Multiple instances are independent
v.x = 100.0
println($"independent: {v.x} {v2.x}")

// _tostring works alongside native fields
println(v2.tostring())

// Increment on int native field
v.w = 5
v.w++
println($"w after ++: {v.w}")
v.w--
println($"w after --: {v.w}")

// Increment on float native field
v.x = 1.5
v.x++
println($"x after ++: {v.x}")
v.x--
println($"x after --: {v.x}")

// Compound assignment
v.w = 10
v.w += 5
println($"w after +=5: {v.w}")
v.x = 2.0
v.x *= 3.0
println($"x after *=3: {v.x}")

// Clone copies native field values
v.x = 11.0
v.y = 22.0
v.z = 33.0
v.w = 44
local vc = clone v
println($"clone: {vc.x} {vc.y} {vc.z} {vc.w}")
vc.x = 0.0
println($"clone independent: {v.x} {vc.x}")

// foreach over instance reads native fields
local v3 = NativeVec(1.0, 2.0, 3.0, 99)
local keys = []
local vals = []
foreach (k, val in v3) {
  keys.append(k)
  vals.append(val)
}
// Native fields should appear in enumeration
println($"foreach has x: {keys.indexof("x") != null}")
println($"foreach has w: {keys.indexof("w") != null}")
println($"foreach x val: {vals[keys.indexof("x")]}")
println($"foreach w val: {vals[keys.indexof("w")]}")

// 'in' operator
println($"x in v: {"x" in v}")
println($"w in v: {"w" in v}")
println($"q in v: {"q" in v}")

// Non-literal key access (goes through _OP_GET/_OP_SET, not _OP_GET_LITERAL/_OP_SET_LITERAL)
local key = "x"
v[key] = 77.0
println($"non-literal get: {v[key]}")

// Type mismatch errors
// Literal path (_OP_SET_LITERAL)
try { v.x = "hello" } catch(e) println($"err literal: {e}")
// Non-literal path (_OP_SET)
try { local k = "x"; v[k] = "hello" } catch(e) println($"err non-literal: {e}")
// Null assignment
try { v.x = null } catch(e) println($"err null: {e}")

println("PASSED")
