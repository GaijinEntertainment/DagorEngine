// EXPECTED: compile-time error - fn returns string|int, but var only accepts int
function fn(val): string|int {
    return val ? 42 : "hello"
}
local x: int = fn(1)
return x
