// EXPECTED: no error - const values match declared types
const MY_INT = 42
const MY_STR = "hello"
const MY_FLOAT = 3.14
const MY_BOOL = true

local a: int = MY_INT
local b: string = MY_STR
local c: float = MY_FLOAT
local d: bool = MY_BOOL
local e: int|string = MY_INT
local f: int|string = MY_STR
return [a, b, c, d, e, f]
