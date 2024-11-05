if (__name__ == "__analysis__")
  return

//expect:w215

local flag = true
local b = 10 + flag ? 1 : 2
::print(b)