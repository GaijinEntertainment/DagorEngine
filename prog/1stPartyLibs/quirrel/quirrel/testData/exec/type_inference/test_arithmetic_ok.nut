// EXPECTED: no error - arithmetic on ints produces int
local a: int = 10
local b: int = 20
local c: int = a + b
local d: int = a * b
local e: int = a - b
local f: int = a / b
local g: int = a % b
return [c, d, e, f, g]
