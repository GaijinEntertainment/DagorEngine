// EXPECTED: compile-time error - return float from int function
function fn(): int {
    return 3.14
}
return fn
