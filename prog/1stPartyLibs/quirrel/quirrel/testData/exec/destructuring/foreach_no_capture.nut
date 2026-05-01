// Regression guard for the zero-overhead path: when the body has no inner
// function or lambda referencing idx/val, the codegen keeps idx/val in the
// foreach's shared slot. Behavior must be identical to pre-capture-fix.

local sum = 0
foreach (v in [1, 2, 3, 4]) sum += v
println("s1:", sum)

local pairs = 0
foreach (i, v in [10, 20, 30]) pairs += i * 100 + v
println("s2:", pairs)

// Nested function defined but doesn't reference outer idx/val: still
// shared-slot path (over-approx may take the per-iter path, output is the
// same either way - this asserts the semantic, not the strategy).
local r = 0
foreach (v in [5, 6]) {
  function helper(x) { return x + 1 }
  r += helper(v)
}
println("s3:", r)

// break/continue still work on shared-slot path.
local first_even = -1
foreach (v in [1, 3, 5, 4, 7]) {
  if (v % 2 != 0) continue
  first_even = v
  break
}
println("s4:", first_even)
