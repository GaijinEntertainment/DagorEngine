if (__name__ == "__analysis__")
  return

function foo(_p) {}

local x = foo(1)


if (x == null && foo(2))
    foo(x.y)
else
    foo(x.x)

