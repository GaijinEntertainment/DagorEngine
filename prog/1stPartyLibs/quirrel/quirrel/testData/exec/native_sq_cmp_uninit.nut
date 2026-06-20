// B9: sq_cmp must return a defined 0 when ObjCmp fails on incomparable
// operands, not uninitialized stack garbage. Needs the native C API.

from "test.native" import raw_cmp

// sq_cmp compares the top two stack slots, so raw_cmp(a,b) reports b<=>a.
println($"cmp ordered: {raw_cmp(3, 5)} {raw_cmp(5, 3)} {raw_cmp(5, 5)}")
println($"cmp incomparable: {raw_cmp(function() {}, 5)}")

println("PASSED")
