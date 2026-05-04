// Test: reassigned local used in ADDI fold (2-instruction fold: LOAD, ADDI)
// The peephole folds LOADINT+MOVE into LOADINT targeting the local's register.
// The ADDI fold must not replace that LOADINT with a result targeting a different register.

// Pattern: local x = A; x = B; local y = x + C + D
// After peephole: LOADINT x_reg B, ADDI y_reg C x_reg, ADDI y_reg D y_reg
// The first ADDI fold must not destroy x's value.

{
  local v1 = -111
  v1 = 3
  local loop2 = v1 - -5 + 12
  assert(v1 == 3, $"expected v1==3, got {v1}")
  assert(loop2 == 20, $"expected loop2==20, got {loop2}")
}

{
  local a = 100
  a = 5
  local b = a + 10
  assert(a == 5, $"expected a==5, got {a}")
  assert(b == 15, $"expected b==15, got {b}")
}

{
  local a = 100
  a = 5
  local b = a + 10 + 20
  assert(a == 5, $"expected a==5, got {a}")
  assert(b == 35, $"expected b==35, got {b}")
}

// Chain of ADDI folds
{
  local x = -1
  x = 7
  local y = x + 1 + 2 + 3
  assert(x == 7, $"expected x==7, got {x}")
  assert(y == 13, $"expected y==13, got {y}")
}

// Negative immediates
{
  local x = 999
  x = 20
  local y = x - 5 - 3
  assert(x == 20, $"expected x==20, got {x}")
  assert(y == 12, $"expected y==12, got {y}")
}

println("opt_reassign_addi: OK")
