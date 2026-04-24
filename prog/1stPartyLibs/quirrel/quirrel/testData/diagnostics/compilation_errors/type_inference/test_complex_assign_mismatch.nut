// EXPECTED: compile-time error
// Variable with type annotation reassigned with wrong type expression
local x: int = 42
x = "hello"
return x
