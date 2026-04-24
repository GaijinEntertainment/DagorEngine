// Test: both loads in 3-instruction fold target the same register.
// When loadA and loadB write to the same register, loadA's value is dead
// (overwritten by loadB). The optimizer must not use loadA's stale value.

// Pattern: local x = A; x = B; local y = x - x
// After peephole: LOADINT reg A, LOADINT reg B, SUB y_reg reg reg
// Both loads target the same register. Result must be B - B = 0, not A - B.

{
  local v1 = -111
  v1 = 12
  local loop2 = v1 - v1
  assert(v1 == 12, $"expected v1==12, got {v1}")
  assert(loop2 == 0, $"expected loop2==0, got {loop2}")
}

{
  local x = 99
  x = 5
  local y = x + x
  assert(x == 5, $"expected x==5, got {x}")
  assert(y == 10, $"expected y==10, got {y}")
}

{
  local x = 99
  x = 7
  local y = x * x
  assert(x == 7, $"expected x==7, got {x}")
  assert(y == 49, $"expected y==49, got {y}")
}

// Division by self
{
  local x = 50
  x = 13
  local y = x / x
  assert(x == 13, $"expected x==13, got {x}")
  assert(y == 1, $"expected y==1, got {y}")
}

// Modulo by self
{
  local x = 50
  x = 13
  local y = x % x
  assert(x == 13, $"expected x==13, got {x}")
  assert(y == 0, $"expected y==0, got {y}")
}

// Bitwise XOR with self
{
  local x = 50
  x = 0xFF
  local y = x ^ x
  assert(x == 0xFF, $"expected x==255, got {x}")
  assert(y == 0, $"expected y==0, got {y}")
}

// Bitwise AND with self
{
  local x = 50
  x = 0xAB
  local y = x & x
  assert(x == 0xAB, $"expected x==171, got {x}")
  assert(y == 0xAB, $"expected y==171, got {y}")
}

println("opt_same_reg_fold: OK")
