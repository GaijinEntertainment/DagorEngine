// EXPECTED: compile-time error
// Complex return expression: function declared to return int,
// but the expression is a ternary with string branch
function compute(flag: bool, x: int, y: int): int {
    return flag ? x * y + 1 : "error"
}
return compute
