if (__name__ == "__analysis__")
  return

// Root table slots - should warn
// Note: usage of the root table is deprecated
{
  ::flags <- 0x2040
  ::aspect_ratio <- (::flags && 0x40)
}

// Binding - should warn
{
  let flags = 0x2040
  let _res = (flags && 0x40)
}


// Local variable - should warn
{
  local flags = 0x2040
  local _res = (flags && 0x40)
}

// Indirect reference to a constant value through an unary operator - should not warn
{
  ::f <- 10
  local mask = -0x40
  ::a <- (::f && !mask)
}

// Not a subject to "const-in-bool-expr" check - should not warn
{
  function bar(_g) {}
  function foo() {}
  let { s, o } = foo()
  const Z = "xxs"

  let _seen = bar(@() o.value && s.value?[Z])
}
