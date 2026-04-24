// EXPECTED: compile-time error - const string assigned to int variable
const MY_STR = "hello"
local x: int = MY_STR
return x
