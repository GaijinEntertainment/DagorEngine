// EXPECTED: compile-time error
// Deeply nested expression with many operators
// ((((1 + 2) * 3) > 5) ? "big" : "small") != "big" => bool
// bool is not subset of string
local a: int = 1
local b: int = 2
local c: int = 3
local deep_result: string = ((((a + b) * c) > 5) ? "big" : "small") != "big"
return deep_result
