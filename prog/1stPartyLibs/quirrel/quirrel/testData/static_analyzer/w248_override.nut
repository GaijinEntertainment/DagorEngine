function foo() {}
local x = foo()?.x


if (foo() || !x)
  return  // After this, x is non-null

let r = x
x = null

if (r.s) // Don't warn because r was assigned when x was non-null
  foo()
