// assert() acts as a null-check

if (__name__ == "__analysis__")
  return


function foo() {}
let o = foo()
let c = o?.x
assert(c != null)
let _g = c.y // No warning - assert guarantees c != null at this point
