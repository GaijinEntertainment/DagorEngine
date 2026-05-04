#allow-compiler-internals

// Test 1: simple return from CodeBlockExpr inside try
try {
  let a = $${ return 42 }
  assert(a == 42, $"expected 42, got {a}")
} catch (e) {
  assert(false, $"unexpected exception: {e}")
}

// Test 2: conditional return from CodeBlockExpr inside try
try {
  let b = $${
    local x = 10
    if (x > 5)
      return x * 2
    return -1
  }
  assert(b == 20, $"expected 20, got {b}")
} catch (e) {
  assert(false, $"unexpected exception: {e}")
}

// Test 3: exception INSIDE CodeBlockExpr is caught by outer try
local caught = false
try {
  let c = $${
    throw "boom"
    return 0
  }
  assert(false, "should not reach here")
} catch (e) {
  caught = true
  assert(e == "boom", $"expected 'boom', got '{e}'")
}
assert(caught, "exception should have been caught")

// Test 4: try inside CodeBlockExpr
let d = $${
  try {
    throw "inner"
    return -1
  } catch (e) {
    return 99
  }
}
assert(d == 99, $"expected 99, got {d}")

// Test 5: try inside CodeBlockExpr inside try
try {
  let e = $${
    try {
      throw "nested"
      return -1
    } catch (ex) {
      return 77
    }
  }
  assert(e == 77, $"expected 77, got {e}")
} catch (ex) {
  assert(false, $"unexpected exception: {ex}")
}

// Test 6: foreach inside CodeBlockExpr inside try
try {
  let f = $${
    local sum = 0
    foreach (v in [1, 2, 3, 4, 5])
      sum += v
    return sum
  }
  assert(f == 15, $"expected 15, got {f}")
} catch (e) {
  assert(false, $"unexpected exception: {e}")
}

// Test 7: null return from CodeBlockExpr inside try
try {
  let g = $${ return null }
  assert(g == null, $"expected null, got {g}")
} catch (e) {
  assert(false, $"unexpected exception: {e}")
}

// Test 8: bare return (no value) from CodeBlockExpr inside try
try {
  let h = $${ return }
  assert(h == null, $"expected null, got {h}")
} catch (e) {
  assert(false, $"unexpected exception: {e}")
}

print("ALL CODEBLOCK+TRY TESTS PASSED\n")
