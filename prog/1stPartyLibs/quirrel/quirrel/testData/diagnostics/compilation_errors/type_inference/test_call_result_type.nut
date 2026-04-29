// EXPECTED: compile-time error - function returns int, assigned to string variable
function sqri(x: int): int {
    return x * x
}
local y: string = sqri(5)
return y
