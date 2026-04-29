// EXPECTED: compile-time error - string concat produces string, not int
local s: string = "hello"
local n: int = 42
local result: int = s + n
return result
