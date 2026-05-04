class SetThrows {
  function _set(key, val) {
    throw "Not supported"
  }
}

class GetThrows {
  function _get(key) {
    throw "Read denied"
  }
}

class ThrowsNonString {
  function _set(key, val) {
    throw 42
  }
}

// Test _set string error
try {
  let x = SetThrows()
  x.foo = 123
  assert(false, "should have thrown")
} catch (e) {
  assert(e == "Error in '_set' metamethod: Not supported", $"unexpected: {e}")
}

// Test _get string error
try {
  let x = GetThrows()
  let _ = x.foo
  assert(false, "should have thrown")
} catch (e) {
  assert(e == "Error in '_get' metamethod: Read denied", $"unexpected: {e}")
}

// Test non-string error object
try {
  let x = ThrowsNonString()
  x.foo = 123
  assert(false, "should have thrown")
} catch (e) {
  assert(e == "Error in '_set' metamethod: '42' (type='integer')", $"unexpected: {e}")
}

println("ALL PASSED")
