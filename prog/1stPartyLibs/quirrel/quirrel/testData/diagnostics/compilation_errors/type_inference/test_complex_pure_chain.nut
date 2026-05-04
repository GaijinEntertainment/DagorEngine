// EXPECTED: compile-time error
// Chain of pure function calls with type mismatch at the end
// sqri returns int, sqrf returns float, fn returns string|int
// sqrf(1.5) + 3 => float (float arithmetic) => not subset of function|null
const function [pure] sqri(x: int): int {
    return x * x
}
const function [pure] sqrf(x: float): float {
    return x * x
}
function fn(val): string|int {
    return val ? sqri(val) : "error"
}

local v: function|null = sqrf(4.3) + 3
return v
