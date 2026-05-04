// EXPECTED: no error - ternary produces int|int = int
local flag = true
local x: int = flag ? 42 : 100
return null
