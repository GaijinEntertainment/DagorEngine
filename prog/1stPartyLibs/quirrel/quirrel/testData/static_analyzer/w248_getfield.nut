function foo() {}

let o = foo()

if (o?.w == null)
    return

o.x = true  // No warning - o is non-null after the early return guard
