// EXPECTED: compile-time error
// Bitwise operations always produce int
// (a & b) | (c ^ d) << 2 => int => not subset of string
local a: int = 0xFF
local b: int = 0x0F
local c: int = 0xAA
local d: int = 0x55
local result: string = (a & b) | (c ^ d) << 2
return result
