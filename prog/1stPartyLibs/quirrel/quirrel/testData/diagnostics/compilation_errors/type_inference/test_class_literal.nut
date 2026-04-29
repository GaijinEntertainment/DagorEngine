// EXPECTED: compile-time error - class literal assigned to string variable
local x: string = class {
    a = 1
    b = 2
}
return x
