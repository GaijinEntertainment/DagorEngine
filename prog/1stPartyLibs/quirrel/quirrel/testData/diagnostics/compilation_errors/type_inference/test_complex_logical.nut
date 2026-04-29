// EXPECTED: compile-time error
// Logical operators: || returns one of the operands (not necessarily bool)
// 42 || "hello" => int|string => not subset of bool
local x: bool = 42 || "hello"
return x
