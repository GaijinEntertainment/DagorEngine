// EXPECTED: no error
// Bitwise operations always produce int
local a: int = 0xFF
local b: int = 0x0F
local c: int = 0xAA
local d: int = 0x55
local result: int = (a & b) | (c ^ d) << 2
local neg: int = ~a
local shr: int = a >>> 4
return [result, neg, shr]
