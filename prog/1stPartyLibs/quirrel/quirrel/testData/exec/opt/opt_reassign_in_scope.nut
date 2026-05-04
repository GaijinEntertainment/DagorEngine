// Test: reassigned locals used across nested scopes and control flow.
// Ensures optimizer respects liveness in more complex code patterns.

// Reassign inside block, use after
{
  local x = 10
  x = 20
  local y = x + 1
  if (y > 0) {
    assert(x == 20, $"expected x==20, got {x}")
  }
  assert(y == 21, $"expected y==21, got {y}")
}

// Reassign, use in loop init
{
  local x = 100
  x = 3
  local sum = 0
  for (local i = x + 1; i < 10; i++)
    sum += i
  assert(x == 3, $"expected x==3, got {x}")
  assert(sum == 4+5+6+7+8+9, $"expected sum==39, got {sum}")
}

// Reassign, use in function call argument
{
  local x = 50
  x = 7
  local arr = [x + 1, x + 2, x + 3]
  assert(x == 7, $"expected x==7, got {x}")
  assert(arr[0] == 8, $"expected arr[0]==8, got {arr[0]}")
  assert(arr[1] == 9, $"expected arr[1]==9, got {arr[1]}")
  assert(arr[2] == 10, $"expected arr[2]==10, got {arr[2]}")
}

// Multiple locals reassigned
{
  local a = 1
  local b = 2
  a = 10
  b = 20
  local c = a + b
  assert(a == 10, $"expected a==10, got {a}")
  assert(b == 20, $"expected b==20, got {b}")
  assert(c == 30, $"expected c==30, got {c}")
}

// Reassign with compound expression using self
{
  local x = 100
  x = 8
  x = x + 2  // x should be 10
  local y = x + 5
  assert(x == 10, $"expected x==10, got {x}")
  assert(y == 15, $"expected y==15, got {y}")
}

println("opt_reassign_in_scope: OK")
