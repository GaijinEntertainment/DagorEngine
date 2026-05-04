// EXPECTED: compile-time error - return string from int function
function fn(): int {
    return "hello"
}
return fn
