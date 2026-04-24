// EXPECTED: no error
// Complex return expression: all branches produce int
function compute(flag: bool, x: int, y: int): int {
    return flag ? x * y + 1 : x - y
}
return compute
