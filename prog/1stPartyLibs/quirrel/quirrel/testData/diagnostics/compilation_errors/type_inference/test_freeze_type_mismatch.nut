// EXPECTED: compile-time error - freeze({}) returns table, assigned to int variable
local x: int = freeze({a = 1})
return x
