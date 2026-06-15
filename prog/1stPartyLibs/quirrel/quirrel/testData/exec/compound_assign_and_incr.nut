// Coverage: bytecode opcodes that were never dispatched by the existing suite.
// Targets _OP_COMPARITH, _OP_INC, _OP_SETI, _OP_SETK, _OP_JCMPF and their
// already-covered siblings, so the interpreter dispatch for each is exercised.

// _OP_COMPARITH: compound assignment via a *computed* (non-literal) key
local arr = [1, 2, 3, 4]
local k = 1
arr[k] += 10        // COMPARITH (+)
arr[k] *= 2         // COMPARITH (*)
arr[k] -= 4         // COMPARITH (-)
println(arr[1])     // ((2+10)*2)-4 = 20

// _OP_COMPARITH_K: compound assignment via a literal field key
local t = { count = 0 }
t.count += 5        // COMPARITH_K
t.count *= 3        // COMPARITH_K
println(t.count)    // 15

// _OP_INC (prefix ++/-- on an access expr) and _OP_PINC (postfix on access)
local box = [0]
local pre = ++box[0]    // INC, returns new value
local post = box[0]++   // PINC, returns old value
println(pre)            // 1
println(post)           // 1
println(box[0])         // 2

++t.count               // INC on field
t.count--               // PINC on field
println(t.count)        // 15

// _OP_INCL / _OP_PINCL: local inc/dec
local n = 5
n++                     // PINCL
++n                     // INCL
n--                     // PINCL
--n                     // INCL
println(n)              // 5

// _OP_SETI: indexed store with int-literal key where value is preloaded
local v = 99
local a2 = [0, 0, 0]
a2[1] = v               // SETI (LOADINT key immediately precedes SET)
println(a2[1])          // 99

// _OP_SETK: field store with string key where value is preloaded
// (slot must already exist: '[]=' is a SET, not a newslot)
local tb = { dyn = 0 }
tb["dyn"] = v           // SETK
println(tb.dyn)         // 99

// _OP_JCMPF: float local compared against a float constant inside a branch.
// Each comparison operator is tested with three cases: operand greater than,
// equal to, and less than the constant 2.5 (i.e. 3.5, 2.5, 1.5).
function cmpGT(x) { if (x >  2.5) return 1; return 0 }
function cmpLT(x) { if (x <  2.5) return 1; return 0 }
function cmpGE(x) { if (x >= 2.5) return 1; return 0 }
function cmpLE(x) { if (x <= 2.5) return 1; return 0 }
function cmpEQ(x) { if (x == 2.5) return 1; return 0 }
function cmpNE(x) { if (x != 2.5) return 1; return 0 }

println($"GT: {cmpGT(3.5)} {cmpGT(2.5)} {cmpGT(1.5)}")   // 1 0 0
println($"LT: {cmpLT(3.5)} {cmpLT(2.5)} {cmpLT(1.5)}")   // 0 0 1
println($"GE: {cmpGE(3.5)} {cmpGE(2.5)} {cmpGE(1.5)}")   // 1 1 0
println($"LE: {cmpLE(3.5)} {cmpLE(2.5)} {cmpLE(1.5)}")   // 0 1 1
println($"EQ: {cmpEQ(3.5)} {cmpEQ(2.5)} {cmpEQ(1.5)}")   // 0 1 0
println($"NE: {cmpNE(3.5)} {cmpNE(2.5)} {cmpNE(1.5)}")   // 1 0 1
