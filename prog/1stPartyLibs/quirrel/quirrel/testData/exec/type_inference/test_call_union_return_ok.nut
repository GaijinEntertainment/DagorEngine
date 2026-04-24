// EXPECTED: no error - fn returns string|int, var accepts string|int|null
function fn(val): string|int {
    return val ? 42 : "hello"
}
local x: string|int = fn(1)
local y: string|int|null = fn(1)
return [x, y]
