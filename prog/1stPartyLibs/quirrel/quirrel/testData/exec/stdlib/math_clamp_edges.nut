// Coverage: math.clamp branches in sqstdmath.cpp - value below range (returns
// lo), above range (returns hi), in range (returns value), and the
// min>max error path.

let math = require("math")

assert(math.clamp(10, 0, 100) == 10)    // in range -> value
assert(math.clamp(-5, 0, 100) == 0)     // below    -> lo
assert(math.clamp(250, 0, 100) == 100)  // above    -> hi
assert(math.clamp(0, 0, 100) == 0)      // on lower bound
assert(math.clamp(100, 0, 100) == 100)  // on upper bound

// float variant
assert(math.clamp(1.5, 0.0, 1.0) == 1.0)
assert(math.clamp(-0.5, 0.0, 1.0) == 0.0)

// error: min greater than max
local threw = false
try { math.clamp(5, 100, 0) } catch (e) { threw = true }
assert(threw, "expected min>max error")

println("clamp edges: OK")
