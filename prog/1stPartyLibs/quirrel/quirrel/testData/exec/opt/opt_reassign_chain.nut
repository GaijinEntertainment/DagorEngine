// Test: chained constant expressions after reassignment.
// Ensures multi-step folding doesn't lose intermediate assignments.

// Two-step chain: x = B; y = x op const1 op const2
{
  local x = 100
  x = 5
  local y = x + 10 + 20
  assert(x == 5, $"expected x==5, got {x}")
  assert(y == 35, $"expected y==35, got {y}")
}

// Subtraction chain
{
  local x = 100
  x = 50
  local y = 100 - x - 10
  assert(x == 50, $"expected x==50, got {x}")
  assert(y == 40, $"expected y==40, got {y}")
}

// Mixed operations
{
  local a = 0
  a = 6
  local b = 0
  b = 7
  local c = a + b
  assert(a == 6, $"expected a==6, got {a}")
  assert(b == 7, $"expected b==7, got {b}")
  assert(c == 13, $"expected c==13, got {c}")
}

// Multiple reassignments before use
{
  local x = 1
  x = 2
  x = 3
  local y = x + 10
  assert(x == 3, $"expected x==3, got {x}")
  assert(y == 13, $"expected y==13, got {y}")
}

// Reassign used in comparison (JCMP fold path)
{
  local x = 100
  x = 5
  local r = x < 10
  assert(x == 5, $"expected x==5, got {x}")
  assert(r, $"expected x < 10 to be true")
}

// Reassign with float
{
  local x = 1.0
  x = 3.0
  local y = x + 1.0
  assert(x == 3.0, $"expected x==3.0, got {x}")
  assert(y == 4.0, $"expected y==4.0, got {y}")
}

// Reassign float, same-register fold
{
  local x = 1.0
  x = 5.0
  local y = x - x
  assert(x == 5.0, $"expected x==5.0, got {x}")
  assert(y == 0.0, $"expected y==0.0, got {y}")
}

// JCMP same-register: both sides of comparison are the same reassigned variable (seed 916086 pattern)
{
  local v0 = -111
  v0 = -3
  if (v0 >= v0)
    assert(true, "v0 >= v0 should be true")
  else
    assert(false, "v0 >= v0 was false - optimizer used stale register value")
}

// JCMP same-register with different initial value
{
  local v0 = 999
  v0 = 42
  if (v0 == v0)
    assert(true, "v0 == v0 should be true")
  else
    assert(false, "v0 == v0 was false")
}

// JCMP same-register: less-than (always false for same value)
{
  local v0 = 100
  v0 = 7
  if (v0 < v0)
    assert(false, "v0 < v0 should be false")
  else
    assert(true, "v0 < v0 is correctly false")
}

println("opt_reassign_chain: OK")
