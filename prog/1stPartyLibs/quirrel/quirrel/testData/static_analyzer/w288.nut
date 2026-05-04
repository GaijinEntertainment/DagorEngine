if (__name__ == "__analysis__")
  return

local fn = function (aaa_x, bbb, ...) {return aaa_x + bbb}

local b = 1
return fn(b)
