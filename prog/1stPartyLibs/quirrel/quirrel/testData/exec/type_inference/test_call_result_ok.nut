// EXPECTED: no error - function return type matches variable type
function sqri(x: int): int {
    return x * x
}
local y: int = sqri(5)
local z: int|null = sqri(5)
local w: int|string = sqri(5)
return [y, z, w]
