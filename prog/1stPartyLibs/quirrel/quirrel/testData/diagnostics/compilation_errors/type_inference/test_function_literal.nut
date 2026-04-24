// EXPECTED: compile-time error - lambda assigned to int variable
local x: int = @() 42
return x
