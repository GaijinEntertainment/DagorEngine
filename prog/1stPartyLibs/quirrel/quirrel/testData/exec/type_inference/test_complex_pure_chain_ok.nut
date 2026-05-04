// EXPECTED: no error
// Pure function results used correctly
const function [pure] sqri(x: int): int {
    return x * x
}
const function [pure] sqrf(x: float): float {
    return x * x
}
function fn(val): string|int {
    return val ? sqri(val) : "error"
}

local a: int = sqri(5)
local b: float = sqrf(3.14)
local c: string|int = fn(42)
local d: string|int|null|function = fn(100)
return [a, b, c, d]
