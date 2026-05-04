// EXPECTED: no error
// Variable with type annotation reassigned with correct type
local x: int = 42
x = 100
x = 1 + 2 * 3
return null
