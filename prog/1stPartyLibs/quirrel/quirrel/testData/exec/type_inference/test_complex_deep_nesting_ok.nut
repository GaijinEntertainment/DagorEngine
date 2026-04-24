// EXPECTED: no error
// Deeply nested expression where types work out
// ((((1 + 2) * 3) > 5) ? "big" : "small") => string (both branches are string)
local a: int = 1
local b: int = 2
local c: int = 3
local deep_result: string = (((a + b) * c) > 5) ? "big" : "small"
return deep_result
