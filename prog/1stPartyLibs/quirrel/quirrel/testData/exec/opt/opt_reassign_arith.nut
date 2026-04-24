// Test: reassigned local used in arithmetic (3-instruction fold: LOAD, LOAD, ARITH)
// The peephole optimizer merges LOADINT+MOVE into a dual-purpose LOADINT.
// The post-pass const folder must not destroy the reassignment side effect.

// Pattern: local x = A; x = B; x -= x / C
// After peephole: LOADINT reg B (also assigns x), LOADINT tmp C, DIV, SUB
// The fold must preserve x = B.
{
  local x = 11
  x = 222
  x -= x / 3333
  assert(x == 222, $"expected 222, got {x}")
}

// Same with addition
{
  local x = 99
  x = 10
  x += x / 5
  assert(x == 12, $"expected 12, got {x}")
}

// Same with multiplication
{
  local x = 99
  x = 7
  x *= x + 1
  // Can't fold x*x at compile time, but ensure x==7
  assert(x == 56, $"expected 56, got {x}")
}

// Reassign then binary op where result goes to different register
{
  local x = 50
  x = 10
  local y = x + 5
  assert(x == 10, $"expected x==10, got {x}")
  assert(y == 15, $"expected y==15, got {y}")
}

// Reassign then binary op with two constants after peephole
{
  local x = 50
  x = 10
  local y = x * 3 + 1
  assert(x == 10, $"expected x==10, got {x}")
  assert(y == 31, $"expected y==31, got {y}")
}

println("opt_reassign_arith: OK")
