// EXPECTED: compile-time error - int arithmetic result assigned to string
local a: int = 10
local b: int = 20
local c: string = a + b
return c
