// EXPECTED: compile-time error - comparison produces bool, assigned to int
local a: int = 10
local b: int = 20
local c: int = a > b
return c
