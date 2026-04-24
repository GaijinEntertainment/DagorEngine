// EXPECTED: compile-time error - nested ternaries produce int|float|string, var only accepts int|float
//   flag1 ? (flag2 ? 42 : 3.14) : "oops"
//   true-branch:  flag2 ? 42 : 3.14  => int|float
//   false-branch: "oops"             => string
//   combined: int|float|string       => NOT subset of int|float
local flag1 = true
local flag2 = false
local x: int|float = flag1 ? (flag2 ? 42 : 3.14) : "oops"
return x
