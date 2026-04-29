// EXPECTED: no error
// Logical operators: || returns one of the operands
// true || false => bool|bool = bool
local x: bool = true || false
local y: int|string = 42 || "hello"
return [x, y]
