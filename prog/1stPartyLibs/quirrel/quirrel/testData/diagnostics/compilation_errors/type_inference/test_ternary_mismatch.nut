// EXPECTED: compile-time error - ternary produces int|string, var only accepts int
local flag = true
local x: int = flag ? 42 : "hello"
return x
