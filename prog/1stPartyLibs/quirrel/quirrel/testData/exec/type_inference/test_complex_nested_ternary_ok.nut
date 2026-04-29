// EXPECTED: no error - nested ternaries produce int|float|string which fits the declared type
local flag1 = true
local flag2 = false
local x: int|float|string = flag1 ? (flag2 ? 42 : 3.14) : "hello"
return x
