// EXPECTED: compile-time error
// Complex arithmetic expression with mixed types ending up assigned to wrong type
// (1 + 2) * 3 => int, typeof "x" => string
// result of ternary: int|string => not subset of bool
function get_flag(): bool { return true }
local n: int = 10
local x: bool = get_flag() ? ((1 + 2) * 3 + n) : typeof "x"
return x
